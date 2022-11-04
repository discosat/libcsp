executable('csp_server_client',
	['csp_server_client.c', 'csp_server_client_posix.c'],
	include_directories : csp_inc,
	c_args : csp_c_args,
	dependencies : csp_dep,
	build_by_default : true)

executable('csp_client',
	['csp_client.c', 'csp_server_client_posix.c'],
	include_directories : csp_inc,
	c_args : csp_c_args,
	dependencies : csp_dep,
	build_by_default : true)

executable('csp_server',
	['csp_server.c', 'csp_server_client_posix.c'],
	include_directories : csp_inc,
	c_args : csp_c_args,
	dependencies : csp_dep,
	build_by_default : true)

executable('csp_arch',
	'csp_arch.c',
	include_directories : csp_inc,
	c_args : csp_c_args,
	dependencies :  csp_dep,
	build_by_default : true)

executable('zmqproxy',
	'zmqproxy.c',
	include_directories : csp_inc,
	c_args : csp_c_args,
	dependencies : csp_dep,
	build_by_default : true)
