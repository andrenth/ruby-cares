#include <ruby.h>
#include <ares.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <netdb.h>

#define BUFLEN	46

static VALUE cNameInfo;
static VALUE cNotImpError;
static VALUE cBadNameError;
static VALUE cNotFoundError;
static VALUE cNoMemError;
static VALUE cDestrError;
static VALUE cFlagsError;

static void
define_cares_exceptions(VALUE cC)
{
	cNotImpError =
	    rb_define_class_under(cC, "NotImplementedError", rb_eException);
	cBadNameError =
	    rb_define_class_under(cC, "BadNameError", rb_eException);
	cNotFoundError =
	    rb_define_class_under(cC, "AddressNotFoundError", rb_eException);
	cNoMemError =
	    rb_define_class_under(cC, "NoMemoryError", rb_eException);
	cDestrError =
	    rb_define_class_under(cC, "DestructionError", rb_eException);
	cFlagsError =
	    rb_define_class_under(cC, "BadFlagsError", rb_eException);
}

static void
define_nameinfo_flags(VALUE cNI)
{
	rb_define_const(cNI, "NOFQDN", INT2NUM(ARES_NI_NOFQDN));
	rb_define_const(cNI, "NUMERICHOST", INT2NUM(ARES_NI_NUMERICHOST));
	rb_define_const(cNI, "NAMEREQD", INT2NUM(ARES_NI_NAMEREQD));
	rb_define_const(cNI, "NUMERICSERV", INT2NUM(ARES_NI_NUMERICSERV));
	rb_define_const(cNI, "TCP", INT2NUM(ARES_NI_TCP));
	rb_define_const(cNI, "UDP", INT2NUM(ARES_NI_UDP));
	rb_define_const(cNI, "SCTP", INT2NUM(ARES_NI_SCTP));
	rb_define_const(cNI, "DCCP", INT2NUM(ARES_NI_DCCP));
	rb_define_const(cNI, "NUMERICSCOPE", INT2NUM(ARES_NI_NUMERICSCOPE));
}

static void
raise_error(int error)
{
	switch (error) {
	case ARES_ENOTIMP:
		rb_raise(cNotImpError, "%s", ares_strerror(error));
		break;
	case ARES_ENOTFOUND:
		rb_raise(cNotFoundError, "%s", ares_strerror(error));
		break;
	case ARES_ENOMEM:
		rb_raise(cNoMemError, "%s", ares_strerror(error));
		break;
	case ARES_EDESTRUCTION:
		rb_raise(cDestrError, "%s", ares_strerror(error));
		break;
	case ARES_EBADFLAGS:
		rb_raise(cFlagsError, "%s", ares_strerror(error));
		break;
	}
}

static VALUE
rb_cares_destroy(void *chp)
{
	ares_destroy(*(ares_channel *)chp);
	return(Qnil);
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

	Data_Get_Struct(self, ares_channel, chp);

	status = ares_init(chp);
	if (status != ARES_SUCCESS)
		raise_error(status);
	return(self);
}

static void
host_callback(void *arg, int status, struct hostent *hp)
{
	char	**p;
	char	  buf[BUFLEN];
	VALUE	  block, info, aliases;

        if (status != ARES_SUCCESS)
                raise_error(status);

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
		rb_raise(rb_eArgError, "gethostbyname: block needed");

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

	switch (cfamily) {
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
		rb_raise(cNotImpError, "gethostbyaddr: invalid address family");
	}
	return(self);
}

static void
nameinfo_callback(void *arg, int status, char *node, char *service)
{
	VALUE	  block, info;

        if (status != ARES_SUCCESS)
                raise_error(status);

	block = (VALUE)arg;

	info = rb_ary_new();

	if (node != NULL)
		rb_ary_push(info, rb_str_new2(node));
	
	if (service != NULL)
		rb_ary_push(info, rb_str_new2(service));
	
	rb_funcall(block, rb_intern("call"), 1, info);
}

static VALUE
rb_cares_getnameinfo(VALUE self, VALUE info)
{
	int	cflags;
	socklen_t sslen;
	struct sockaddr_storage ss;
	ares_channel *chp;
	VALUE	vaddr, vport, vflags;	/* Hash keys */

	Data_Get_Struct(self, ares_channel, chp);

	vflags = rb_hash_aref(info, ID2SYM(rb_intern("cflags")));
	if (!NIL_P(vflags))
		cflags = NUM2INT(vflags);
	else
		cflags = 0;

	sslen = 0;
	ss.ss_family = AF_INET;

	vaddr = rb_hash_aref(info, ID2SYM(rb_intern("addr")));
	if (!NIL_P(vaddr)) {
		char	*caddr = StringValuePtr(vaddr);
		struct sockaddr_in *sinp;
		struct sockaddr_in6 *sin6p;

		sinp  = (struct sockaddr_in *)&ss;
		sin6p = (struct sockaddr_in6 *)&ss;

		cflags |= ARES_NI_LOOKUPHOST;

		if (inet_pton(AF_INET, caddr, &sinp->sin_addr) == 1) {
			sslen = sizeof(struct sockaddr_in);
		} else if (inet_pton(AF_INET6, caddr, &sin6p->sin6_addr) == 1) {
			ss.ss_family = AF_INET6;
			sslen = sizeof(struct sockaddr_in6);
		} else {
			rb_raise(cNotImpError,
				 "getnameinfo: invalid address family");
		}
	}

	vport = rb_hash_aref(info, ID2SYM(rb_intern("port")));
	if (!NIL_P(vport)) {
		u_short	cport = htons(NUM2UINT(vport));
		cflags |= ARES_NI_LOOKUPSERVICE;
		switch (ss.ss_family) {
		case AF_INET:
			((struct sockaddr_in *)&ss)->sin_port = cport;
			sslen = sizeof(struct sockaddr_in);
			break;
		case AF_INET6:
			((struct sockaddr_in6 *)&ss)->sin6_port = cport;
			sslen = sizeof(struct sockaddr_in6);
			break;
		}
	}

	ares_getnameinfo(*chp, (struct sockaddr *)&ss, sslen, cflags,
			 nameinfo_callback, (void *)rb_block_proc());
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

void
Init_cares(void)
{
	VALUE cCares = rb_define_class("Cares", rb_cObject);
	define_cares_exceptions(cCares);

	cNameInfo = rb_define_class_under(cCares, "NameInfo", rb_cObject);
	define_nameinfo_flags(cNameInfo);

	rb_define_alloc_func(cCares, rb_cares_alloc);
	rb_define_method(cCares, "initialize", rb_cares_init, 0);
	rb_define_method(cCares, "gethostbyname", rb_cares_gethostbyname, 2);
	rb_define_method(cCares, "gethostbyaddr", rb_cares_gethostbyaddr, 2);
	rb_define_method(cCares, "getnameinfo", rb_cares_getnameinfo, 1);
	rb_define_method(cCares, "select_loop", rb_cares_select_loop, 0);
}
