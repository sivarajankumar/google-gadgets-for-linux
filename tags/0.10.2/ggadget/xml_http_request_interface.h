/*
  Copyright 2008 Google Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#ifndef GGADGET_XML_HTTP_REQUEST_INTERFACE_H__
#define GGADGET_XML_HTTP_REQUEST_INTERFACE_H__

#include <ggadget/scriptable_interface.h>

namespace ggadget {

template <typename R> class Slot0;
class Connection;
class DOMDocumentInterface;
class XMLParserInterface;

/**
 * References:
 *   - http://www.w3.org/TR/XMLHttpRequest/
 *   - http://msdn.microsoft.com/library/default.asp?url=/library/en-us/xmlsdk/html/xmobjxmlhttprequest.asp
 *   - http://developer.mozilla.org/cn/docs/XMLHttpRequest
 */
class XMLHttpRequestInterface : public ScriptableInterface {
 public:
  CLASS_ID_DECL(0x301dceaec56141d6);

  enum ExceptionCode {
    NO_ERR = 0,
    INVALID_STATE_ERR = 11,
    SYNTAX_ERR = 12,
    SECURITY_ERR = 18,
    NETWORK_ERR = 101,
    ABORT_ERR = 102,
    NULL_POINTER_ERR = 200,
    OTHER_ERR = 300,
  };

  enum State {
    UNSENT,
    OPENED,
    HEADERS_RECEIVED,
    LOADING,
    DONE,
  };

 protected:
  virtual ~XMLHttpRequestInterface() { }

 public:
  virtual Connection *ConnectOnReadyStateChange(Slot0<void> *handler) = 0;
  virtual State GetReadyState() = 0;

  virtual ExceptionCode Open(const char *method, const char *url, bool async,
                             const char *user, const char *password) = 0;
  virtual ExceptionCode SetRequestHeader(const char *header,
                                         const char *value) = 0;
  virtual ExceptionCode Send(const char *data, size_t size) = 0;
  virtual ExceptionCode Send(const DOMDocumentInterface *data) = 0;
  virtual void Abort() = 0;

  virtual ExceptionCode GetAllResponseHeaders(const char **result) = 0;
  virtual ExceptionCode GetResponseHeader(const char *header,
                                          const char **result) = 0;
  virtual ExceptionCode GetResponseText(const char **result) = 0;
  virtual ExceptionCode GetResponseBody(const char **result, size_t *size) = 0;
  virtual ExceptionCode GetResponseXML(DOMDocumentInterface **result) = 0;
  virtual ExceptionCode GetStatus(unsigned short *result) = 0;
  virtual ExceptionCode GetStatusText(const char **result) = 0;

  /** Convenient alternative of GetResponseBody(const char **, size_t *). */
  virtual ExceptionCode GetResponseBody(std::string *result) = 0;
};

CLASS_ID_IMPL(XMLHttpRequestInterface, ScriptableInterface)

/**
 * The factory interface used to create @c XMLHttpRequestInterface instances.
 */
class XMLHttpRequestFactoryInterface {
 public:
  virtual ~XMLHttpRequestFactoryInterface() { }

  /**
   * Creates a new @c XMLHttpRequestInterface session.
   * @return the session id, or -1 on failure. This method should never
   *     return 0.
   */
  virtual int CreateSession() = 0;

  /**
   * Destroys a session. All @c XMLHttpRequestInterface instance created in
   * this session must have been deleted before this method is called.
   * @param session_id the session id.
   */
  virtual void DestroySession(int session_id) = 0;

  /**
   * Creates a @c XMLHttpRequestInterface instance in a session.
   * @param session_id the session id. All @c XMLHttpRequest instances created
   *     in the same session share the same set of cookies.
   *     If session_id is 0, no cookie will be shared for the returned instance.
   * @param parser the XML parser for this instance.
   * @return @c XMLHttpRequestInterface instance, or @c NULL on failure.
   */
  virtual XMLHttpRequestInterface *CreateXMLHttpRequest(
      int session_id, XMLParserInterface *parser) = 0;

  /**
   * Sets the default User-Agent value. It will be sent if no User-Agent
   * header is set for a @c XMLHttpRequest instance.
   */
  virtual void SetDefaultUserAgent(const char *user_agent) = 0;
};

/**
 * Sets an @c XMLHttpRequestFactory as the global XMLHttpRequest factory.
 * An XMLHttpRequest extension module can call this function in its
 * @c Initailize() function.
 */
bool SetXMLHttpRequestFactory(XMLHttpRequestFactoryInterface *factory);

/**
 * Gets the global XMLHttpRequest factory.
 */
XMLHttpRequestFactoryInterface *GetXMLHttpRequestFactory();

} // namespace ggadget

#endif // GGADGET_XML_HTTP_REQUEST_INTERFACE_H__