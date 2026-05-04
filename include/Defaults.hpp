#pragma once

#ifndef InstallDir
# define InstallDir "/usr/local"
#endif

#define DefaultConfig	InstallDir "/webserv/conf/webserv.conf"
#define HtmlDir			InstallDir "/webserv/html"

#define SUPPORTED_CGI_EXTENSIONS { ".py", ".php" }

#define MAX_REQUEST_LINE_SIZE	8192	// 8KB
#define MAX_HEADER_SIZE			16384	// 16KB
#define MAX_URI_SIZE			(MAX_REQUEST_LINE_SIZE - 15)
