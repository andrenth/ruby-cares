#include <ruby.h>
#include <ares.h>
#include <netdb.h>

#define BUFLEN	46

static VALUE
rb_cares_destroy(void *chp)
{
	ares_destroy(*(ares_channel *)chp);
}

static VALUE
rb_cares_alloc(VALUE klass)
{
	ares_channel *chp;
	return(Data_Make_Struct(klass, ares_channel, 0, rb_cares_destroy, chp));
}

static VALUE
rb_cares_init(VALUE self)
{
	int	status;
	ares_channel *chp;
	VALUE	obj;

	Data_Get_Struct(self, ares_channel, chp);

	status = ares_init(chp);
	if (status != ARES_SUCCESS)
		rb_raise(rb_eRuntimeError, "%s", ares_strerror(status));
	return(self);
}

static void
host_callback(void *arg, int status, struct hostent *hp)
{
	char	**p;
	char	  buf[BUFLEN];
	VALUE	  block, info, aliases;

        if (status != ARES_SUCCESS)
                rb_raise(rb_eRuntimeError, "%s", ares_strerror(status));

	block = (VALUE)arg;

	info = rb_ary_new();
	rb_ary_push(info, rb_str_new2(hp->h_name));
	
	aliases = rb_ary_new();
	rb_ary_push(info, aliases);
	if (hp->h_aliases != NULL) {
		for (p = hp->h_aliases; *p; p++)
			rb_ary_push(aliases, rb_str_new2(*p));
	}

	rb_ary_push(info, INT2NUM(hp->h_addrtype));

	for (p = hp->h_addr_list; *p != NULL; p++) {
		inet_ntop(hp->h_addrtype, *p, buf, BUFLEN);
		rb_ary_push(info, rb_str_new2(buf));
	}

	rb_funcall(block, rb_intern("call"), 1, info);
}

static VALUE
rb_cares_gethostbyname(VALUE self, VALUE host, VALUE family)
{
	ares_channel *chp;

	if (!rb_block_given_p())
		rb_raise(rb_eArgError, "gethostbyname needs a block");

	Data_Get_Struct(self, ares_channel, chp);
	ares_gethostbyname(*chp, StringValuePtr(host), NUM2INT(family),
			   host_callback, (void *)rb_block_proc());
	return(self);
}

static VALUE
rb_cares_gethostbyaddr(VALUE self, VALUE addr, VALUE family)
{
	char	*caddr;
	int	 cfamily;
	ares_channel *chp;

	if (!rb_block_given_p())
		rb_raise(rb_eArgError, "gethostbyaddr: block needed");

	Data_Get_Struct(self, ares_channel, chp);
	cfamily = NUM2INT(family);
	caddr = StringValuePtr(addr);

	switch(cfamily) {
	case AF_INET: {
		struct in_addr in;
		if (inet_pton(cfamily, caddr, &in) != 1)
			rb_sys_fail("gethostbyaddr");
		ares_gethostbyaddr(*chp, &in, sizeof(in), cfamily,
				   host_callback, (void *)rb_block_proc());
		break;
	}
	case AF_INET6: {
		struct in6_addr in6;
		if (inet_pton(cfamily, caddr, &in6) != 1)
			rb_sys_fail("gethostbyaddr");
		ares_gethostbyaddr(*chp, &in6, sizeof(in6), cfamily,
				   host_callback, (void *)rb_block_proc());
		break;
	}
	default:
		rb_raise(rb_eArgError, "gethostbyaddr: invalid address family");
	}
	return(self);
}

static VALUE
rb_cares_select_loop(VALUE self)
{
	int	nfds;
	fd_set	read, write;
	struct timeval *tvp, tv;
	ares_channel *chp;

	Data_Get_Struct(self, ares_channel, chp);

	for (;;) {
		FD_ZERO(&read);
		FD_ZERO(&write);
		nfds = ares_fds(*chp, &read, &write);
		if (nfds == 0)
			break;
		tvp = ares_timeout(*chp, NULL, &tv); /* XXX max timeout */
		rb_thread_select(nfds, &read, &write, NULL, tvp);
		ares_process(*chp, &read, &write);
	}
	return(Qnil);
}

static VALUE
rb_cares_getnameinfo(VALUE self, VALUE addr)
{
	/* TODO */
}

void
Init_cares(void)
{
	VALUE cCares = rb_define_class("Cares", rb_cObject);
	rb_define_alloc_func(cCares, rb_cares_alloc);
	rb_define_method(cCares, "initialize", rb_cares_init, 0);
	rb_define_method(cCares, "gethostbyname", rb_cares_gethostbyname, 2);
	rb_define_method(cCares, "gethostbyaddr", rb_cares_gethostbyaddr, 2);
	rb_define_method(cCares, "getnameinfo", rb_cares_getnameinfo, 2);
	rb_define_method(cCares, "select_loop", rb_cares_select_loop, 0);
}
