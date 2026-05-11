#pragma once

#ifndef InstallDir
# define InstallDir "/usr/local"
#endif

#define DefaultConfig	InstallDir "/webserv/conf/webserv.conf"
#define HtmlDir			InstallDir "/webserv/html"

#define SUPPORTED_CGI_EXTENSIONS { ".py", ".php" }

#define MAX_REQUEST_LINE_SIZE	(1 << 13)	// 8KB
#define MAX_HEADER_SIZE			(1 << 14)	// 16KB
#define MAX_URI_SIZE			(MAX_REQUEST_LINE_SIZE - 15)

#define CLIENT_READ_BUFFER_SIZE	(1 << 16)	// 64 KB
#define CLIENT_RCVBUF_SIZE		(1 << 20)	// 1 MB

#define SERVER_SOFTWARE			"webserv/1.0"
