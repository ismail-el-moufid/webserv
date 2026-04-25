#include "utils/MimeUtils.hpp"

std::string MimeUtils::mimeByPath(const std::string &path) {
    // rfind: Find last occurrence of content in string.
    size_t dot = path.rfind('.')
    if (dot == std::string::nopos) {
        return "application/octet-stream";
    }

    std::string ext = path.substrt(dot);
    if (ext == ".png")  return "image/png";
	if (ext == ".jpg")  return "image/jpeg";
	if (ext == ".gif")  return "image/gif";
	if (ext == ".svg")  return "image/svg+xml";
    if (ext == ".html") return "text/html";
	if (ext == ".css")  return "text/css";
	if (ext == ".js")   return "application/javascript";
	if (ext == ".json") return "application/json";
	if (ext == ".xml")  return "application/xml";
	if (ext == ".txt")  return "text/plain";
	if (ext == ".ico")  return "image/x-icon";
	if (ext == ".pdf")  return "application/pdf";

    return "application/octet-stream";
}