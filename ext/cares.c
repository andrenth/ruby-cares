#include <ruby.h>
#include <ares.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <netdb.h>

#define BUFLEN	46

static VALUE cInit;
static VALUE cNameInfo;
static VALUE cNotImpError;
static VALUE cBadNameError;
static VALUE cNotFoundError;
static VALUE cNoMemError;
static VALUE cDestrError;
static VALUE cFlagsError;

static void
define_cares_exceptions(VALUE cares)
{
	cNotImpError =
	    rb_define_class_under(cares, "NotImplementedError", rb_eException);
	cBadNameError =
	    rb_define_class_under(cares, "BadNameError", rb_eException);
	cNotFoundError =
	    rb_define_class_under(cares, "AddressNotFoundError", rb_eException);
	cNoMemError =
	    rb_define_class_under(cares, "NoMemoryError", rb_eException);
	cDestrError =
	    rb_define_class_under(cares, "DestructionError", rb_eException);
	cFlagsError =
	    rb_define_class_under(cares, "BadFlagsError", rb_eException);
}

static void
define_init_flags(VALUE init)
{
	rb_define_const(init, "USEVC", INT2NUM(ARES_FLAG_USEVC));
	rb_define_const(init, "PRIMARY", INT2NUM(ARES_FLAG_PRIMARY));
	rb_define_const(init, "IGNTC", INT2NUM(ARES_FLAG_IGNTC));
	rb_define_const(init, "NORECURSE", INT2NUM(ARES_FLAG_NORECURSE));
	rb_define_const(init, "STAYOPEN", INT2NUM(ARES_FLAG_STAYOPEN));
	rb_define_const(init, "NOSEARCH", INT2NUM(ARES_FLAG_NOSEARCH));
	rb_define_const(init, "NOALIASES", INT2NUM(ARES_FLAG_NOALIASES));
	rb_define_const(init, "NOCHECKRESP", INT2NUM(ARES_FLAG_NOCHECKRESP));
}

static void
define_nameinfo_flags(VALUE ni)
{
	rb_define_const(ni, "NOFQDN", INT2NUM(ARES_NI_NOFQDN));
	rb_define_const(ni, "NUMERICHOST", INT2NUM(ARES_NI_NUMERICHOST));
	rb_define_const(ni, "NAMEREQD", INT2NUM(ARES_NI_NAMEREQD));
	rb_define_const(ni, "NUMERICSERV", INT2NUM(ARES_NI_NUMERICSERV));
	rb_define_const(ni, "TCP", INT2NUM(ARES_NI_TCP));
	rb_define_const(ni, "UDP", INT2NUM(ARES_NI_UDP));
	rb_define_const(ni, "SCTP", INT2NUM(ARES_NI_SCTP));
	rb_define_const(ni, "DCCP", INT2NUM(ARES_NI_DCCP));
	rb_define_const(ni, "NUMERICSCOPE", INT2NUM(ARES_NI_NUMERICSCOPE));
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

static void
init_callback(void *arg, int s, int read, int write)
{
	VALUE	socket, rb_cSock;
	VALUE	block = (VALUE)arg;

	rb_cSock = rb_const_get(rb_cObject, rb_intern("Socket"));
	socket = rb_funcall(rb_cSock, rb_intern("for_fd"), 1, INT2NUM(s));

	rb_funcall(block, rb_intern("call"), 3, socket,
		   read ? Qtrue : Qfalse, write ? Qtrue : Qfalse);
}

static int
set_init_opts(VALUE opts, struct ares_options *aop)
{
	int	optmask = 0;
	VALUE	vflags, vtimeout, vtries, vndots, vudp_port, vtcp_port;
	VALUE	vservers, vdomains, vlookups;

	if (!NIL_P(opts)) {
		vflags = rb_hash_aref(opts, ID2SYM(rb_intern("flags")));
		if (!NIL_P(vflags)) {
			aop->flags = NUM2INT(vflags);
			optmask |= ARES_OPT_FLAGS;
		}
		vtimeout = rb_hash_aref(opts, ID2SYM(rb_intern("timeout")));
		if (!NIL_P(vtimeout)) {
			aop->timeout = NUM2INT(vtimeout);
			optmask |= ARES_OPT_TIMEOUT;
		}
		vtries = rb_hash_aref(opts, ID2SYM(rb_intern("tries")));
		if (!NIL_P(vtries)) {
			aop->timeout = NUM2INT(vtries);
			optmask |= ARES_OPT_TRIES;
		}
		vndots = rb_hash_aref(opts, ID2SYM(rb_intern("ndots")));
		if (!NIL_P(vndots)) {
			aop->timeout = NUM2INT(vtries);
			optmask |= ARES_OPT_NDOTS;
		}
		vudp_port = rb_hash_aref(opts, ID2SYM(rb_intern("udp_port")));
		if (!NIL_P(vudp_port)) {
			aop->udp_port = NUM2UINT(vudp_port);
			optmask |= ARES_OPT_UDP_PORT;
		}
		vtcp_port = rb_hash_aref(opts, ID2SYM(rb_intern("tcp_port")));
		if (!NIL_P(vtcp_port)) {
			aop->tcp_port = NUM2UINT(vtcp_port);
			optmask |= ARES_OPT_TCP_PORT;
		}
		vservers = rb_hash_aref(opts, ID2SYM(rb_intern("servers")));
		if (!NIL_P(vservers)) {
			long	i, n;
			struct in_addr in, *servers;

			n = RARRAY(vservers)->len;
			servers = ALLOCA_N(struct in_addr, n);
			for (i = 0; i < n; i++) {
				char	*caddr;
				VALUE	 vaddr;
				vaddr = rb_ary_entry(vservers, i);
				caddr = StringValuePtr(vaddr);
				if (inet_pton(AF_INET, caddr, &in) != 1)
					rb_sys_fail("initialize");
				MEMCPY(servers+i, &in, struct in_addr, 1);
			}
			aop->servers = servers;
			aop->nservers = n;
			optmask |= ARES_OPT_SERVERS;
		}
		vdomains = rb_hash_aref(opts, ID2SYM(rb_intern("domains")));
		if (!NIL_P(vdomains)) {
			char	*domains;
			long	 i, n;

			n = RARRAY(vdomains)->len;
			domains = ALLOC_N(char, n);
			for (i = 0; i < n; i++) {
				char	*cdomain;
				VALUE	 vdomain;
				vdomain = rb_ary_entry(vdomains, i);
				cdomain = StringValuePtr(vdomain);
				MEMCPY(domains+i, cdomain, strlen(cdomain), 1);
			}
			aop->domains = &domains;
			aop->ndomains = n;
			optmask |= ARES_OPT_DOMAINS;
		}
		vlookups = rb_hash_aref(opts, ID2SYM(rb_intern("lookups")));
		if (!NIL_P(vlookups)) {
			ID	id = rb_to_id(vlookups);
			if (id == rb_intern("dns"))
				aop->lookups = "b";
			else if (id == rb_intern("hosts"))
				aop->lookups = "f";
			else {
				/*
				 * XXX Let c-ares handle it (why does rb_raise
				 * segfault here?)
				 * rb_raise(rb_eArgError, "initialize: "
				 *	    "invalid value for :lookups");
				 */
				aop->lookups = "";
			}
			optmask |= ARES_OPT_LOOKUPS;
		}
	}
	if (rb_block_given_p()) {
		aop->sock_state_cb = init_callback;
		aop->sock_state_cb_data = (void *)rb_block_proc();
		optmask |= ARES_OPT_SOCK_STATE_CB;
	}

	return(optmask);
}

/*
 *  call-seq:
 *     Cares.new([options])                                   => cares_obj
 *     Cares.new([options]) { |socket, read, write| block }   => cares_obj
 *
 *  Creates a new <code>Cares</code> object. The <code>options</code> hash
 *  accepts the following keys:
 *
 *  * <code>:flags</code> Flags controlling the behaviour of the resolver.
 *    See below for a description os the possible flags.
 *  * <code>:timeout</code> The number of seconds each name server is given
 *    to respond to a query on the first try. For further queries, the timeout
 *    scales nearly with the provided value. The default is 5 seconds.
 *  * <code>:tries</code> The number of times the resolver will try to
 *    contact each name server before giving up. The default is 4 tries.
 *  * <code>:ndots</code> The number of dots that must be present in a
 *    domain name for it to be queried "as is", prior to querying with the
 *    default domain extensions appended. The default is 1, unless set
 *    otherwise in resolv.conf or the <code>RES_OPTIONS</code> environment
 *    variable.
 *  * <code>:udp_port</code> The port to use for UDP queries. The default is 53.
 *  * <code>:tcp_port</code> The port to use for TCP queries. The default is 53.
 *  * <code>:servers</code>  An array of IP addresses to be used as the servers
 *    to be contacted, instead of the ones found in resolv.conf or the local
 *    name daemon.
 *  * <code>:domains</code> An array of domains to be searched, instead of
 *    the ones specified in resolv.conf or the machine hostname.
 *  * <code>:lookups</code> The lookups to perform for host queries. It
 *    should be a string of the characters <i>b</i> or <i>f</i>, where <i>b</i>
 *    indicates a DNS lookup, and <i>f</i> indicates a hosts file lookup.
 *
 *  The <code>:flags</code> option is a bitwise-or of values from the list
 *  below:
 *
 *  * <code>Cares::Init::USEVC</code> Always use TCP queries. Normally, TCP
 *    is only used if a UDP query returns a truncated result.
 *  * <code>Cares::Init::PRIMARY</code> Only query the first server in the
 *    server list.
 *  * <code>Cares::Init::IGNTC</code> If a truncated response to an UDP query
 *    is received, do not fall back to TCP; simply continue with the truncated
 *    response.
 *  * <code>Cares::Init::NORECURSE</code> Do not set the "recursion desired"
 *    bit on outgoing queries.
 *  * <code>Cares::Init::STAYOPEN</code> Do not close the communication
 *    sockets when the number of active queries drops to zero.
 *  * <code>Cares::Init::NOSEARCH</code> Do not use the default search
 *    domains; only query hostnames as-is or as aliases.
 *  * <code>Cares::Init::NOALIASES</code> Do not honor the
 *    <code>HOSTALIASES</code> environment variable, which specifies a
 *    file of hostname translations.
 *  * <code>Cares::Init::NOCHECKRESP</code> Do not discard responses with the
 *    <code>SERVFAIL</code>, <code>NOTIMP</code>, or <code>REFUSED</code>
 *    response codes or responses whose questions don't match the
 *    questions in the request.
 *
 *  If a block is given, it'll be called when a socket used in name resolving
 *  has its state changed. The block takes three arguments. The first one is
 *  the <code>Socket</code> object, the the other two are boolean values
 *  indicating if the socket should listen for read and/or write events,
 *  respectively.
 */
static VALUE
rb_cares_init(int argc, VALUE *argv, VALUE self)
{
	int	status, optmask;
	ares_channel *chp;
	struct ares_options ao;
	VALUE	opts;

	Data_Get_Struct(self, ares_channel, chp);

	rb_scan_args(argc, argv, "01", &opts);
	if (NIL_P(opts) && !rb_block_given_p()) {
		status = ares_init(chp);
		if (status != ARES_SUCCESS)
			raise_error(status);
		return(self);
	}

	optmask = set_init_opts(opts, &ao);

	status = ares_init_options(chp, &ao, optmask);
	if (status != ARES_SUCCESS)
		raise_error(status);

	return(self);
}

static void
host_callback(void *arg, int status, struct hostent *hp)
{
	char	  buf[BUFLEN], **p;
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

/*
 *  call-seq:
 *     cares.gethostbyname(name, family) { |cname, aliases, family, *addrs| block }  => cares
 *
 *  Performs a DNS lookup on <code>name</code>, for addresses of family
 *  <code>family</code>. The <code>family</code> argument is either
 *  <code>Socket::AF_INET</code> or <code>Socket::AF_INET6</code>. The results
 *  are passed as arguments to the block:
 *
 *  * <code>cname</code>: <code>name</code>'s canonical name.
 *  * <code>aliases</code>: array of aliases.
 *  * <code>family</code>: address family.
 *  * <code>*addrs</code>: array containing <code>name</code>'s addresses.
 */
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

/*
 *  call-seq:
 *     cares.gethostbyaddr(addr, family) { |name, aliases, family, *addrs| block }  => cares
 *
 *  Performs a reverse DNS query on <code>addr</code>. for addresses of family
 *  <code>family</code>. The <code>family</code> argument is either
 *  <code>Socket::AF_INET</code> or <code>Socket::AF_INET6</code>. The results
 *  are passed as arguments to the block:
 *
 *  * <code>name</code>: <code>addr</code>'s name.
 *  * <code>aliases</code>: array of aliases.
 *  * <code>family</code>: address family.
 *  * <code>*addrs</code>: array containing <code>name</code>'s addresses.
 */
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

/*
 *  call-seq:
 *     cares.getnameinfo(nameservice) { |name, service| block }    => cares
 *
 *  Performs a protocol-independent reverse lookup on the host and/or service
 *  names specified on the <code>nameservice</code> hash. The valid keys are:
 *
 *  * <code>:addr</code>: IPv4 or IPv6 address.
 *  * <code>:service</code>: Service name.
 *
 *  The lookup results are passed as parameters to the block.
 */
static VALUE
rb_cares_getnameinfo(VALUE self, VALUE info)
{
	int	cflags;
	socklen_t sslen;
	struct sockaddr_storage ss;
	ares_channel *chp;
	VALUE	vaddr, vport, vflags;

	Data_Get_Struct(self, ares_channel, chp);

	vflags = rb_hash_aref(info, ID2SYM(rb_intern("flags")));
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

/*
 *  call-seq:
 *     cares.select_loop(timeout=nil)    => nil
 *
 *  Handles the input/output and timeout associated witht the queries made
 *  from the name and address lookup methods. The block passed to each of
 *  those methods is yielded when the event is processed.
 *
 *  The <code>timeout</code> hash accepts the following keys:
 *
 *  * <code>:seconds</code>:  Timeout in seconds.
 *  * <code>:useconds</code>: Timeout in microseconds.
 */
static VALUE
rb_cares_select_loop(int argc, VALUE *argv, VALUE self)
{
	int	nfds;
	fd_set	read, write;
	struct timeval *tvp, tv;
	struct timeval *maxtvp, maxtv;
	ares_channel *chp;
	VALUE	timeout;

	rb_scan_args(argc, argv, "01", &timeout);

	maxtvp = NULL;
	if (!NIL_P(timeout)) {
		VALUE	secs, usecs;

		secs = rb_hash_aref(timeout, ID2SYM(rb_intern("seconds")));
		if (!NIL_P(secs))
			maxtv.tv_sec = NUM2LONG(secs);
		usecs = rb_hash_aref(timeout, ID2SYM(rb_intern("useconds")));
		if (!NIL_P(usecs))
			maxtv.tv_usec = NUM2LONG(usecs);

		if (!NIL_P(secs) || !NIL_P(usecs))
			maxtvp = &maxtv;
	}

	Data_Get_Struct(self, ares_channel, chp);

	for (;;) {
		FD_ZERO(&read);
		FD_ZERO(&write);
		nfds = ares_fds(*chp, &read, &write);
		if (nfds == 0)
			break;
		tvp = ares_timeout(*chp, maxtvp, &tv);
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

	cInit = rb_define_class_under(cCares, "Init", rb_cObject);
	define_init_flags(cInit);

	cNameInfo = rb_define_class_under(cCares, "NameInfo", rb_cObject);
	define_nameinfo_flags(cNameInfo);

	rb_define_alloc_func(cCares, rb_cares_alloc);
	rb_define_method(cCares, "initialize", rb_cares_init, -1);
	rb_define_method(cCares, "gethostbyname", rb_cares_gethostbyname, 2);
	rb_define_method(cCares, "gethostbyaddr", rb_cares_gethostbyaddr, 2);
	rb_define_method(cCares, "getnameinfo", rb_cares_getnameinfo, 1);
	rb_define_method(cCares, "select_loop", rb_cares_select_loop, -1);
}
