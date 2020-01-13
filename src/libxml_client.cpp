#include <libxml2/libxml/tree.h>
#include <libxml2/libxml/HTMLparser.h>
#include <libxml++-5.0/libxml++/libxml++.h>

using namespace std;

#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>

#include <iostream>
#include <string>
#include <sstream>

#define HEADER_ACCEPT "Accept:text/html,application/xhtml+xml,application/xml"
#define HEADER_USER_AGENT "User-Agent:Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.17 (KHTML, like Gecko) Chrome/24.0.1312.70 Safari/537.17"

int main()
{
    std::string url = "https://www.cnblogs.com/";
    curlpp::Easy request;

    // Specify the URL
    request.setOpt(curlpp::options::Url(url));

    // Specify some headers
    std::list<std::string> headers;
    headers.push_back(HEADER_ACCEPT);
    headers.push_back(HEADER_USER_AGENT);
    request.setOpt(new curlpp::options::HttpHeader(headers));
    request.setOpt(new curlpp::options::FollowLocation(true));

    // Configure curlpp to use stream
    std::ostringstream responseStream;
    curlpp::options::WriteStream streamWriter(&responseStream);
    request.setOpt(streamWriter);

    // Collect response
    request.perform();
    std::string re = responseStream.str();

    // std::cout << re << std::endl;
    // Parse HTML and create a DOM tree
    xmlDoc *doc = htmlReadDoc((xmlChar *)re.c_str(), NULL, NULL, HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);

    // Encapsulate raw libxml document in a libxml++ wrapper
    xmlNode *r = xmlDocGetRootElement(doc);
    xmlpp::Element *root = new xmlpp::Element(r);

    // Grab the post item bady
    std::string xpath = "//div[@class='post_item_body']";

    std::cout << "标题 \t\t\t\t\t\t\t\t"
              << "阅读量\t" << endl;

    auto elements = root->find(xpath);
    for (const auto &e : elements)
    {
        auto title = e->find("child::h3/a/text()");
        auto view_amount = e->find("child::div/span[@class='article_view']/a/text()");

        cout << dynamic_cast<xmlpp::ContentNode *>(title[0])->get_content() << "\t\t"
             << dynamic_cast<xmlpp::ContentNode *>(view_amount[0])->get_content() << std::endl;
    }

    delete root;
    xmlFreeDoc(doc);

    return 0;
}