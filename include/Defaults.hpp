#pragma once

#ifndef PREFIX
# define PREFIX "/usr/local"
#endif

#define DefaultConfig	PREFIX "/webserv/conf/webserv.conf"
#define HtmlDir			PREFIX "/webserv/html"
#define SESSION_DIR		PREFIX "/webserv/sessions"

#define SUPPORTED_CGI_EXTENSIONS { ".py", ".php" }

#define MAX_REQUEST_LINE_SIZE	(1 << 13)	// 8KB
#define MAX_HEADER_SIZE			(1 << 14)	// 16KB
#define MAX_URI_SIZE			(MAX_REQUEST_LINE_SIZE - 15)

#define CLIENT_READ_BUFFER_SIZE	(1 << 16)	// 64 KB
#define CLIENT_RCVBUF_SIZE		(1 << 20)	// 1 MB

#define CLIENT_SNDBUF_SIZE		(1 << 13)	// 8 KB

#define SESSION_TIMEOUT			3			// senconds

#define SERVER_SOFTWARE			"webserv/1.0"
