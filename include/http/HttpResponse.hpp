#pragma once

#include "http/HttpStatusCodes.hpp" // Code

#include <string>					// string
#include <map>						// map

class HttpResponse
{
	public:

		HttpResponse();

		void init();

		void setStatus(HttpStatus::Code status);
		void setHeader(const std::string &name, const std::string &value);
		void setContentType(const std::string &type);
		void setBody(const std::string &body);
		void setFile(const std::string &path, size_t size);

		bool				hasFile()	const;
		const std::string	&filePath()	const;

		void reset();

		// serialize: build the complete response.
		std::string serialize() const;

		HttpStatus::Code statusCode() const;

		static HttpResponse HttpErrorResponse(HttpStatus::Code status);
		static HttpResponse HttpResponseRedirect(HttpStatus::Code status, const std::string &location);
		static HttpResponse HttpResponseBuilder(HttpStatus::Code status, const std::string &body, const std::string &contentType);

	private:

		HttpStatus::Code					status_;
		std::map<std::string, std::string>	headers_;
		std::string							body_;
		std::string							filePath_;

};
