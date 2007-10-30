/*
  Copyright 2007 Google Inc.

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

#include <iostream>
#include <locale.h>

#include "ggadget/xml_dom.h"
#include "ggadget/xml_utils.h"
#include "unittest/gunit.h"

using namespace ggadget;

void TestBlankNode(DOMNodeInterface *node) {
  EXPECT_TRUE(node->GetFirstChild() == NULL);
  EXPECT_TRUE(node->GetLastChild() == NULL);
  EXPECT_TRUE(node->GetPreviousSibling() == NULL);
  EXPECT_TRUE(node->GetNextSibling() == NULL);
  EXPECT_TRUE(node->GetParentNode() == NULL);
  EXPECT_FALSE(node->HasChildNodes());

  DOMNodeListInterface *children = node->GetChildNodes();
  EXPECT_EQ(0U, children->GetLength());
  children->Destroy();
}

void TestChildren(DOMNodeInterface *parent, DOMNodeListInterface *children,
                  size_t num_child, ...) {
  ASSERT_EQ(num_child, children->GetLength());

  if (num_child == 0) {
    EXPECT_TRUE(parent->GetFirstChild() == NULL);
    EXPECT_TRUE(parent->GetLastChild() == NULL);
    children->Destroy();
    return;
  }

  va_list ap;
  va_start(ap, num_child);
  for (size_t i = 0; i < num_child; i++) {
    DOMNodeInterface *expected_child = va_arg(ap, DOMNodeInterface *);

    if (i == 0) {
      EXPECT_TRUE(parent->GetFirstChild() == expected_child);
      EXPECT_TRUE(expected_child->GetPreviousSibling() == NULL);
    } else {
      EXPECT_TRUE(expected_child->GetPreviousSibling() ==
                  children->GetItem(i - 1));
    }

    if (i == num_child - 1) {
      EXPECT_TRUE(parent->GetLastChild() == expected_child);
      EXPECT_TRUE(expected_child->GetNextSibling() == NULL);
    } else {
      EXPECT_TRUE(expected_child->GetNextSibling() == children->GetItem(i + 1));
    }

    EXPECT_TRUE(expected_child->GetParentNode() == parent);
    EXPECT_TRUE(children->GetItem(i) == expected_child);
  }
  ASSERT_TRUE(children->GetItem(num_child) == NULL);
  ASSERT_TRUE(children->GetItem(num_child * 2) ==  NULL);
  ASSERT_TRUE(children->GetItem(static_cast<size_t>(-1)) == NULL);
  va_end(ap);
}

void TestNullNodeValue(DOMNodeInterface *node) {
  EXPECT_TRUE(node->GetNodeValue() == NULL);
  node->SetNodeValue("abcde");
  EXPECT_TRUE(node->GetNodeValue() == NULL);
}

TEST(XMLDOM, TestBlankDocument) {
  DOMDocumentInterface *doc = CreateDOMDocument();
  ASSERT_TRUE(doc);
  EXPECT_STREQ(kDOMDocumentName, doc->GetNodeName());
  EXPECT_EQ(DOMNodeInterface::DOCUMENT_NODE, doc->GetNodeType());
  EXPECT_TRUE(doc->GetOwnerDocument() == NULL);
  EXPECT_TRUE(doc->GetAttributes() == NULL);
  TestBlankNode(doc);
  TestNullNodeValue(doc);
  EXPECT_TRUE(doc->GetDocumentElement() == NULL);
  delete doc;
}

TEST(XMLDOM, TestBlankElement) {
  DOMDocumentInterface *doc = CreateDOMDocument();
  doc->Attach();
  DOMElementInterface *root_ele;
  ASSERT_EQ(DOM_NO_ERR, doc->CreateElement("root", &root_ele));
  EXPECT_STREQ("root", root_ele->GetNodeName());
  EXPECT_STREQ("root", root_ele->GetTagName());
  EXPECT_EQ(DOMNodeInterface::ELEMENT_NODE, root_ele->GetNodeType());
  TestBlankNode(root_ele);
  TestNullNodeValue(root_ele);
  EXPECT_TRUE(root_ele->GetOwnerDocument() == doc);
  EXPECT_TRUE(doc->GetDocumentElement() == NULL);
  ASSERT_EQ(DOM_NO_ERR, doc->AppendChild(root_ele));
  EXPECT_TRUE(doc->GetDocumentElement() == root_ele);

  DOMElementInterface *ele1 = root_ele;
  ASSERT_EQ(DOM_INVALID_CHARACTER_ERR, doc->CreateElement("&*(", &ele1));
  ASSERT_TRUE(ele1 == NULL);
  ASSERT_EQ(DOM_INVALID_CHARACTER_ERR, doc->CreateElement(NULL, &ele1));
  ASSERT_EQ(DOM_INVALID_CHARACTER_ERR, doc->CreateElement("", &ele1));
  doc->Detach();
}

TEST(XMLDOM, TestAttrSelf) {
  DOMDocumentInterface *doc = CreateDOMDocument();
  doc->Attach();
  DOMAttrInterface *attr;
  ASSERT_EQ(DOM_NO_ERR, doc->CreateAttribute("attr", &attr));
  EXPECT_STREQ("attr", attr->GetNodeName());
  EXPECT_STREQ("attr", attr->GetName());
  EXPECT_EQ(DOMNodeInterface::ATTRIBUTE_NODE, attr->GetNodeType());
  TestBlankNode(attr);
  EXPECT_TRUE(attr->GetAttributes() == NULL);
  EXPECT_STREQ("", attr->GetNodeValue());
  EXPECT_STREQ("", attr->GetValue());
  EXPECT_STREQ("", attr->GetTextContent());
  attr->SetNodeValue("value1");
  EXPECT_STREQ("value1", attr->GetNodeValue());
  EXPECT_STREQ("value1", attr->GetValue());
  EXPECT_STREQ("value1", attr->GetTextContent());
  attr->SetValue("value2");
  EXPECT_STREQ("value2", attr->GetNodeValue());
  EXPECT_STREQ("value2", attr->GetValue());
  EXPECT_STREQ("value2", attr->GetTextContent());
  attr->SetTextContent("value3");
  EXPECT_STREQ("value3", attr->GetNodeValue());
  EXPECT_STREQ("value3", attr->GetValue());
  EXPECT_STREQ("value3", attr->GetTextContent());
  EXPECT_TRUE(attr->GetOwnerDocument() == doc);

  DOMAttrInterface *attr1 = attr;
  ASSERT_EQ(DOM_INVALID_CHARACTER_ERR, doc->CreateAttribute("&*(", &attr1));
  ASSERT_TRUE(attr1 == NULL);
  ASSERT_EQ(DOM_INVALID_CHARACTER_ERR,
            doc->CreateAttribute("Invalid^Name", &attr1));
  ASSERT_EQ(DOM_INVALID_CHARACTER_ERR, doc->CreateAttribute(NULL, &attr1));
  ASSERT_EQ(DOM_INVALID_CHARACTER_ERR, doc->CreateAttribute("", &attr1));
  delete attr;
  doc->Detach();
}

TEST(XMLDOM, TestParentChild) {
  DOMDocumentInterface *doc = CreateDOMDocument();
  doc->Attach();
  DOMElementInterface *root_ele;
  ASSERT_EQ(DOM_NO_ERR, doc->CreateElement("root", &root_ele));
  DOMNodeListInterface *children = root_ele->GetChildNodes();
  ASSERT_EQ(DOM_NO_ERR, doc->AppendChild(root_ele));
  LOG("No child");
  TestChildren(root_ele, children, 0);

  DOMElementInterface *ele1;
  ASSERT_EQ(DOM_NO_ERR, doc->CreateElement("ele1", &ele1));
  ASSERT_EQ(DOM_NO_ERR, root_ele->AppendChild(ele1));
  LOG("Children: ele1");
  TestChildren(root_ele, children, 1, ele1);

  DOMElementInterface *ele2;
  ASSERT_EQ(DOM_NO_ERR, doc->CreateElement("ele2", &ele2));
  ASSERT_EQ(DOM_NO_ERR, root_ele->AppendChild(ele2));
  LOG("Children: ele1, ele2");
  TestChildren(root_ele, children, 2, ele1, ele2);

  DOMElementInterface *ele3;
  ASSERT_EQ(DOM_NO_ERR, doc->CreateElement("ele3", &ele3));
  ASSERT_EQ(DOM_NO_ERR, root_ele->InsertBefore(ele3, ele1));
  LOG("Children: ele3, ele1, ele2");
  TestChildren(root_ele, children, 3, ele3, ele1, ele2);

  ASSERT_EQ(DOM_NO_ERR, root_ele->InsertBefore(ele3, ele3));
  LOG("Children: ele3, ele1, ele2");
  TestChildren(root_ele, children, 3, ele3, ele1, ele2);

  ASSERT_EQ(DOM_NO_ERR, root_ele->InsertBefore(ele3, NULL));
  LOG("Children: ele1, ele2, ele3");
  TestChildren(root_ele, children, 3, ele1, ele2, ele3);

  ASSERT_EQ(DOM_NO_ERR, root_ele->ReplaceChild(ele3, ele3));
  LOG("Children: ele1, ele2, ele3");
  TestChildren(root_ele, children, 3, ele1, ele2, ele3);

  ASSERT_EQ(DOM_NO_ERR, root_ele->ReplaceChild(ele3, ele2));
  LOG("Children: ele1, ele3");
  TestChildren(root_ele, children, 2, ele1, ele3);

  ASSERT_EQ(DOM_NO_ERR, root_ele->RemoveChild(ele3));
  LOG("Children: ele1");
  TestChildren(root_ele, children, 1, ele1);

  ASSERT_EQ(DOM_NO_ERR, root_ele->RemoveChild(ele1));
  LOG("No Child");
  TestChildren(root_ele, children, 0);

  children->Destroy();
  doc->Detach();
}

TEST(XMLDOM, TestParentChildErrors) {
  DOMDocumentInterface *doc = CreateDOMDocument();
  doc->Attach();

  DOMElementInterface *root_ele;
  ASSERT_EQ(DOM_NO_ERR, doc->CreateElement("root", &root_ele));
  ASSERT_EQ(DOM_NO_ERR, doc->AppendChild(root_ele));

  ASSERT_EQ(DOM_NULL_POINTER_ERR, root_ele->AppendChild(NULL));
  ASSERT_EQ(DOM_NULL_POINTER_ERR, root_ele->InsertBefore(NULL, NULL));
  ASSERT_EQ(DOM_NULL_POINTER_ERR, root_ele->RemoveChild(NULL));
  ASSERT_EQ(DOM_NULL_POINTER_ERR, root_ele->ReplaceChild(NULL, NULL));

  DOMElementInterface *ele1;
  ASSERT_EQ(DOM_NO_ERR, doc->CreateElement("ele1", &ele1));
  ASSERT_EQ(DOM_NO_ERR, root_ele->AppendChild(ele1));
  ASSERT_EQ(DOM_NULL_POINTER_ERR, root_ele->ReplaceChild(NULL, ele1));
  ASSERT_EQ(DOM_NULL_POINTER_ERR, root_ele->ReplaceChild(ele1, NULL));

  DOMElementInterface *ele2;
  doc->CreateElement("ele2", &ele2);
  ASSERT_EQ(DOM_NOT_FOUND_ERR, root_ele->RemoveChild(ele2));
  ASSERT_EQ(DOM_NOT_FOUND_ERR, doc->RemoveChild(ele1));
  ASSERT_EQ(DOM_NOT_FOUND_ERR, ele2->RemoveChild(root_ele));
  ASSERT_EQ(DOM_NOT_FOUND_ERR, root_ele->InsertBefore(ele2, ele2));
  ASSERT_EQ(DOM_NOT_FOUND_ERR, ele2->InsertBefore(ele1, root_ele));
  ASSERT_EQ(DOM_NOT_FOUND_ERR, root_ele->ReplaceChild(ele2, ele2));
  ASSERT_EQ(DOM_NOT_FOUND_ERR, ele2->ReplaceChild(ele1, root_ele));

  DOMNodeInterface *ele2a = ele2->CloneNode(true);
  ASSERT_EQ(DOM_NO_ERR, ele1->AppendChild(ele2));
  ASSERT_EQ(DOM_NO_ERR, ele2->AppendChild(ele2a));

  ASSERT_EQ(DOM_HIERARCHY_REQUEST_ERR, ele2->AppendChild(ele2));
  ASSERT_EQ(DOM_HIERARCHY_REQUEST_ERR, ele2->AppendChild(ele1));
  ASSERT_EQ(DOM_HIERARCHY_REQUEST_ERR, ele2->AppendChild(root_ele));
  ASSERT_EQ(DOM_HIERARCHY_REQUEST_ERR, ele2->InsertBefore(ele2, NULL));
  ASSERT_EQ(DOM_HIERARCHY_REQUEST_ERR, ele2->InsertBefore(ele1, NULL));
  ASSERT_EQ(DOM_HIERARCHY_REQUEST_ERR, ele2->InsertBefore(root_ele, NULL));
  ASSERT_EQ(DOM_HIERARCHY_REQUEST_ERR, ele2->InsertBefore(ele2, ele2a));
  ASSERT_EQ(DOM_HIERARCHY_REQUEST_ERR, ele2->InsertBefore(ele1, ele2a));
  ASSERT_EQ(DOM_HIERARCHY_REQUEST_ERR, ele2->InsertBefore(root_ele, ele2a));
  ASSERT_EQ(DOM_HIERARCHY_REQUEST_ERR, ele2->ReplaceChild(ele2, ele2a));
  ASSERT_EQ(DOM_HIERARCHY_REQUEST_ERR, ele2->ReplaceChild(ele1, ele2a));
  ASSERT_EQ(DOM_HIERARCHY_REQUEST_ERR, ele2->ReplaceChild(root_ele, ele2a));

  DOMDocumentInterface *doc1 = CreateDOMDocument();
  doc1->Attach();
  DOMElementInterface *ele3;
  doc1->CreateElement("ele3", &ele3);
  ASSERT_EQ(DOM_WRONG_DOCUMENT_ERR, root_ele->AppendChild(ele3));
  ASSERT_EQ(DOM_WRONG_DOCUMENT_ERR, root_ele->InsertBefore(ele3, ele1));
  ASSERT_EQ(DOM_WRONG_DOCUMENT_ERR, root_ele->ReplaceChild(ele3, ele1));

  delete ele3;
  doc1->Detach();
  doc->Detach();
}

void TestAttributes(DOMElementInterface *ele, DOMNamedNodeMapInterface *attrs,
                    size_t num_attr, ...) {
  std::string log = "Attrs: ";
  va_list ap;
  va_start(ap, num_attr);
  for (size_t i = 0; i < num_attr; i++) {
    const char *name = va_arg(ap, const char *);
    const char *value = va_arg(ap, const char *);
    log += name;
    log += ':';
    log += value;
    log += ' ';
  }
  LOG("%s", log.c_str());
  va_end(ap);

  ASSERT_EQ(num_attr, attrs->GetLength());

  va_start(ap, num_attr);
  for (size_t i = 0; i < num_attr; i++) {
    const char *name = va_arg(ap, const char *);
    const char *value = va_arg(ap, const char *);
    DOMAttrInterface *attr = down_cast<DOMAttrInterface *>(attrs->GetItem(i));
    EXPECT_STREQ(value, ele->GetAttribute(name));
    EXPECT_STREQ(name, attr->GetName());
    EXPECT_STREQ(value, attr->GetValue());
    EXPECT_TRUE(attr->GetOwnerElement() == ele);
    EXPECT_TRUE(ele->GetAttributeNode(name) == attr);
    EXPECT_TRUE(attrs->GetNamedItem(name) == attr);
  }

  ASSERT_TRUE(attrs->GetItem(num_attr) == NULL);
  ASSERT_TRUE(attrs->GetItem(num_attr * 2) ==  NULL);
  ASSERT_TRUE(attrs->GetItem(static_cast<size_t>(-1)) == NULL);

  va_end(ap);
}

TEST(XMLDOM, TestElementAttr) {
  DOMDocumentInterface *doc = CreateDOMDocument();
  doc->Attach();
  DOMElementInterface *ele;
  ASSERT_EQ(DOM_NO_ERR, doc->CreateElement("root", &ele));
  DOMNamedNodeMapInterface *attrs = ele->GetAttributes();
  ASSERT_EQ(DOM_NO_ERR, doc->AppendChild(ele));

  TestAttributes(ele, attrs, 0);
  ele->SetAttribute("attr1", "value1");
  TestAttributes(ele, attrs, 1, "attr1", "value1");
  ele->SetAttribute("attr1", "value1a");
  TestAttributes(ele, attrs, 1, "attr1", "value1a");
  ele->SetAttribute("attr2", "value2");
  TestAttributes(ele, attrs, 2, "attr1", "value1a", "attr2", "value2");
  ele->SetAttribute("attr1", "value1b");
  ele->SetAttribute("attr2", "value2a");
  TestAttributes(ele, attrs, 2, "attr1", "value1b", "attr2", "value2a");

  DOMAttrInterface *attr1;
  ASSERT_EQ(DOM_NO_ERR, doc->CreateAttribute("attr1", &attr1));
  attr1->SetValue("value1c");
  ASSERT_EQ(DOM_NO_ERR, ele->SetAttributeNode(attr1));
  TestAttributes(ele, attrs, 2, "attr2", "value2a", "attr1", "value1c");

  ASSERT_EQ(DOM_NO_ERR, ele->SetAttributeNode(attr1));
  TestAttributes(ele, attrs, 2, "attr2", "value2a", "attr1", "value1c");

  DOMAttrInterface *attr3;
  ASSERT_EQ(DOM_NO_ERR, doc->CreateAttribute("attr3", &attr3));
  attr3->SetValue("value3");
  ASSERT_EQ(DOM_NO_ERR, ele->SetAttributeNode(attr3));
  TestAttributes(ele, attrs, 3, "attr2", "value2a", "attr1", "value1c",
                 "attr3", "value3");

  ASSERT_EQ(DOM_NO_ERR, ele->RemoveAttributeNode(attr3));
  TestAttributes(ele, attrs, 2, "attr2", "value2a", "attr1", "value1c");

  ele->RemoveAttribute("not-exists");
  TestAttributes(ele, attrs, 2, "attr2", "value2a", "attr1", "value1c");
  ele->RemoveAttribute(NULL);
  TestAttributes(ele, attrs, 2, "attr2", "value2a", "attr1", "value1c");
  ele->RemoveAttribute("attr2");
  TestAttributes(ele, attrs, 1, "attr1", "value1c");
  ele->RemoveAttribute("attr1");
  TestAttributes(ele, attrs, 0);
  ele->RemoveAttribute("not-exists");
  TestAttributes(ele, attrs, 0);

  attrs->Destroy();
  doc->Detach();
}

TEST(XMLDOM, TestElementAttributes) {
  DOMDocumentInterface *doc = CreateDOMDocument();
  doc->Attach();
  DOMElementInterface *ele;
  ASSERT_EQ(DOM_NO_ERR, doc->CreateElement("root", &ele));
  DOMNamedNodeMapInterface *attrs = ele->GetAttributes();
  ASSERT_EQ(DOM_NO_ERR, doc->AppendChild(ele));

  TestAttributes(ele, attrs, 0);
  ele->SetAttribute("attr1", "value1");
  ele->SetAttribute("attr2", "value2");
  TestAttributes(ele, attrs, 2, "attr1", "value1", "attr2", "value2");

  DOMAttrInterface *attr1;
  ASSERT_EQ(DOM_NO_ERR, doc->CreateAttribute("attr1", &attr1));
  attr1->SetValue("value1c");
  ASSERT_EQ(DOM_NO_ERR, attrs->SetNamedItem(attr1));
  TestAttributes(ele, attrs, 2, "attr2", "value2", "attr1", "value1c");

  ASSERT_EQ(DOM_NO_ERR, attrs->SetNamedItem(attr1));
  TestAttributes(ele, attrs, 2, "attr2", "value2", "attr1", "value1c");

  DOMAttrInterface *attr3;
  ASSERT_EQ(DOM_NO_ERR, doc->CreateAttribute("attr3", &attr3));
  attr3->SetValue("value3");
  ASSERT_EQ(DOM_NO_ERR, attrs->SetNamedItem(attr3));
  TestAttributes(ele, attrs, 3, "attr2", "value2", "attr1", "value1c",
                 "attr3", "value3");

  ASSERT_EQ(DOM_NO_ERR, attrs->RemoveNamedItem("attr3"));
  TestAttributes(ele, attrs, 2, "attr2", "value2", "attr1", "value1c");

  ASSERT_TRUE(attrs->GetNamedItem("not-exist") == NULL);
  attrs->RemoveNamedItem("attr2");
  TestAttributes(ele, attrs, 1, "attr1", "value1c");
  attrs->RemoveNamedItem("attr1");
  TestAttributes(ele, attrs, 0);
  attrs->RemoveNamedItem("not-exists");
  TestAttributes(ele, attrs, 0);
  ASSERT_TRUE(attrs->GetNamedItem("not-exist") == NULL);

  attrs->Destroy();
  doc->Detach();
}

TEST(XMLDOM, TestElementAttrErrors) {
  DOMDocumentInterface *doc = CreateDOMDocument();
  doc->Attach();
  DOMElementInterface *ele;
  ASSERT_EQ(DOM_NO_ERR, doc->CreateElement("root", &ele));
  DOMNamedNodeMapInterface *attrs = ele->GetAttributes();
  ASSERT_EQ(DOM_NO_ERR, doc->AppendChild(ele));

  TestAttributes(ele, attrs, 0);
  ele->SetAttribute("attr1", "value1");
  ele->SetAttribute("attr2", "value2");

  DOMAttrInterface *fake_attr2;
  ASSERT_EQ(DOM_NO_ERR, doc->CreateAttribute("attr2", &fake_attr2));
  fake_attr2->SetValue("value2");
  ASSERT_EQ(DOM_NOT_FOUND_ERR, ele->RemoveAttributeNode(fake_attr2));
  TestAttributes(ele, attrs, 2, "attr1", "value1", "attr2", "value2");
  ASSERT_EQ(DOM_HIERARCHY_REQUEST_ERR, ele->AppendChild(fake_attr2));
  delete fake_attr2;

  ele->SetAttribute("attr2", NULL);
  TestAttributes(ele, attrs, 2, "attr1", "value1", "attr2", "");

  ASSERT_EQ(DOM_NOT_FOUND_ERR, attrs->RemoveNamedItem("not-exist"));
  TestAttributes(ele, attrs, 2, "attr1", "value1", "attr2", "");

  ASSERT_EQ(DOM_NULL_POINTER_ERR, ele->SetAttributeNode(NULL));
  ASSERT_EQ(DOM_NULL_POINTER_ERR, attrs->SetNamedItem(NULL));
  ASSERT_EQ(DOM_NULL_POINTER_ERR, ele->RemoveAttributeNode(NULL));
  ASSERT_EQ(DOM_NULL_POINTER_ERR, attrs->RemoveNamedItem(NULL));

  ASSERT_EQ(DOM_INVALID_CHARACTER_ERR, ele->SetAttribute("&*(", "abcde"));
  ASSERT_EQ(DOM_INVALID_CHARACTER_ERR, ele->SetAttribute(NULL, "abcde"));
  ASSERT_EQ(DOM_INVALID_CHARACTER_ERR, ele->SetAttribute("", "abcde"));

  DOMElementInterface *ele1;
  ASSERT_EQ(DOM_NO_ERR, doc->CreateElement("root", &ele1));
  ele1->SetAttribute("attr1", "value1d");
  ASSERT_EQ(DOM_INUSE_ATTRIBUTE_ERR,
            attrs->SetNamedItem(ele1->GetAttributeNode("attr1")));
  ASSERT_EQ(DOM_INUSE_ATTRIBUTE_ERR,
            ele->SetAttributeNode(ele1->GetAttributeNode("attr1")));
  DOMAttrInterface *cloned = down_cast<DOMAttrInterface *>(
      ele1->GetAttributeNode("attr1")->CloneNode(false));
  ASSERT_EQ(DOM_NO_ERR, attrs->SetNamedItem(cloned));
  ASSERT_EQ(DOM_NO_ERR, ele->SetAttributeNode(cloned));
  TestAttributes(ele, attrs, 2, "attr2", "", "attr1", "value1d");
  delete ele1;

  DOMDocumentInterface *doc1 = CreateDOMDocument();
  doc1->Attach();
  DOMAttrInterface *attr_doc1;
  ASSERT_EQ(DOM_NO_ERR, doc1->CreateAttribute("attr_doc1", &attr_doc1));
  ASSERT_EQ(DOM_WRONG_DOCUMENT_ERR, attrs->SetNamedItem(attr_doc1));
  ASSERT_EQ(DOM_WRONG_DOCUMENT_ERR, ele->SetAttributeNode(attr_doc1));
  TestAttributes(ele, attrs, 2, "attr2", "", "attr1", "value1d");
  delete attr_doc1;
  doc1->Detach();

  attrs->Destroy();
  doc->Detach();
}

void TestBlankNodeList(DOMNodeListInterface *list) {
  ASSERT_EQ(0U, list->GetLength());
  ASSERT_TRUE(list->GetItem(0) == NULL);
  ASSERT_TRUE(list->GetItem(static_cast<size_t>(-1)) == NULL);
  ASSERT_TRUE(list->GetItem(1) == NULL);
}

TEST(XMLDOM, TestBlankGetElementsByTagName) {
  DOMDocumentInterface *doc = CreateDOMDocument();
  doc->Attach();
  DOMNodeListInterface *elements = doc->GetElementsByTagName(NULL);
  LOG("Blank document NULL name");
  TestBlankNodeList(elements);
  elements->Destroy();

  elements = doc->GetElementsByTagName("");
  LOG("Blank document blank name");
  TestBlankNodeList(elements);
  elements->Destroy();

  elements = doc->GetElementsByTagName("*");
  LOG("Blank document wildcard name");
  TestBlankNodeList(elements);
  elements->Destroy();

  elements = doc->GetElementsByTagName("not-exist");
  LOG("Blank document non-existent name");
  TestBlankNodeList(elements);
  elements->Destroy();

  doc->Detach();
}

TEST(XMLDOM, TestAnyGetElementsByTagName) {
  const char *xml =
    "<root>"
    " <s/>"
    " <s1><s/></s1>\n"
    " <s><s><s/></s></s>\n"
    " <s><s1><s1/></s1></s>\n"
    "</root>";

  DOMDocumentInterface *doc = CreateDOMDocument();
  doc->Attach();
  ASSERT_TRUE(ParseXMLIntoDOM(xml, "FILENAME", doc));
  DOMNodeListInterface *elements = doc->GetElementsByTagName(NULL);
  LOG("Non-blank document NULL name");
  TestBlankNodeList(elements);
  elements->Destroy();

  elements = doc->GetElementsByTagName("");
  LOG("Non-blank document blank name");
  TestBlankNodeList(elements);
  elements->Destroy();

  elements = doc->GetElementsByTagName("not-exist");
  LOG("Non-blank document non-existant name");
  TestBlankNodeList(elements);
  elements->Destroy();

  elements = doc->GetElementsByTagName("*");
  LOG("Non-blank document wildcard name");
  ASSERT_EQ(10U, elements->GetLength());
  ASSERT_TRUE(elements->GetItem(10U) == NULL);
  ASSERT_TRUE(elements->GetItem(0U) == doc->GetDocumentElement());
  DOMNodeInterface *node = elements->GetItem(4U);
  ASSERT_TRUE(node->GetParentNode() == doc->GetDocumentElement());
  ASSERT_STREQ("s", node->GetNodeName());
  ASSERT_EQ(DOMNodeInterface::ELEMENT_NODE, node->GetNodeType());
  ASSERT_EQ(DOM_NO_ERR, doc->GetDocumentElement()->RemoveChild(node));
  ASSERT_EQ(7U, elements->GetLength());
  ASSERT_TRUE(elements->GetItem(7U) == NULL);
  ASSERT_EQ(DOM_NO_ERR, doc->RemoveChild(doc->GetDocumentElement()));
  TestBlankNodeList(elements);
  elements->Destroy();

  doc->Detach();
}

TEST(XMLDOM, TestGetElementsByTagName) {
  const char *xml =
    "<root>"
    " <s/>"
    " <s1><s/></s1>\n"
    " <s><s><s/></s></s>\n"
    " <s><s1><s1/></s1></s>\n"
    "</root>";

  DOMDocumentInterface *doc = CreateDOMDocument();
  doc->Attach();
  ASSERT_TRUE(ParseXMLIntoDOM(xml, "FILENAME", doc));
  DOMNodeListInterface *elements = doc->GetElementsByTagName("s");
  LOG("Non-blank document name 's'");
  ASSERT_EQ(6U, elements->GetLength());
  ASSERT_TRUE(elements->GetItem(6U) == NULL);
  for (size_t i = 0; i < 6U; i++) {
    DOMNodeInterface *node = elements->GetItem(i);
    ASSERT_STREQ("s", node->GetNodeName());
    ASSERT_EQ(DOMNodeInterface::ELEMENT_NODE, node->GetNodeType());
  }

  ASSERT_EQ(DOM_NO_ERR,
            elements->GetItem(2U)->RemoveChild(elements->GetItem(3U)));
  ASSERT_EQ(4U, elements->GetLength());
  for (size_t i = 0; i < 4U; i++) {
    DOMNodeInterface *node = elements->GetItem(i);
    ASSERT_STREQ("s", node->GetNodeName());
    ASSERT_EQ(DOMNodeInterface::ELEMENT_NODE, node->GetNodeType());
  }

  ASSERT_TRUE(elements->GetItem(4U) == NULL);
  ASSERT_EQ(DOM_NO_ERR, doc->RemoveChild(doc->GetDocumentElement()));
  TestBlankNodeList(elements);
  elements->Destroy();

  doc->Detach();
}

TEST(XMLDOM, TestText) {
  DOMDocumentInterface *doc = CreateDOMDocument();
  doc->Attach();

  static UTF16Char data[] = { 'd', 'a', 't', 'a', 0 };
  DOMTextInterface *text = doc->CreateTextNode(data);

  UTF16String blank_utf16;
  EXPECT_TRUE(UTF16String(text->GetData()) == data);

  EXPECT_STREQ(kDOMTextName, text->GetNodeName());
  TestBlankNode(text);
  EXPECT_STREQ("data", text->GetNodeValue());
  EXPECT_STREQ("data", text->GetTextContent());
  text->SetNodeValue(NULL);
  EXPECT_STREQ("", text->GetNodeValue());
  EXPECT_STREQ("", text->GetTextContent());
  EXPECT_TRUE(blank_utf16 == text->GetData());
  text->SetTextContent("data1");
  EXPECT_STREQ("data1", text->GetNodeValue());
  EXPECT_STREQ("data1", text->GetTextContent());

  text->SetData(data);
  EXPECT_STREQ("data", text->GetNodeValue());
  EXPECT_TRUE(UTF16String(text->GetData()) == data);

  UTF16Char *data_out = data;
  EXPECT_EQ(DOM_NO_ERR, text->SubstringData(0, 5, &data_out));
  EXPECT_TRUE(UTF16String(data_out) == data);
  delete [] data_out;
  EXPECT_EQ(DOM_INDEX_SIZE_ERR, text->SubstringData(5, 0, &data_out));
  EXPECT_TRUE(data_out == NULL);
  EXPECT_EQ(DOM_NO_ERR, text->SubstringData(0, 4, &data_out));
  EXPECT_TRUE(UTF16String(data_out) == data);
  delete [] data_out;
  EXPECT_EQ(DOM_NO_ERR, text->SubstringData(1, 2, &data_out));
  UTF16Char expected_data[] = { 'a', 't', 0 };
  EXPECT_TRUE(UTF16String(expected_data) == data_out);
  delete [] data_out;
  EXPECT_EQ(DOM_NO_ERR, text->SubstringData(1, 0, &data_out));
  EXPECT_TRUE(blank_utf16 == data_out);
  delete [] data_out;

  text->AppendData(NULL);
  EXPECT_TRUE(UTF16String(text->GetData()) == data);
  text->AppendData(blank_utf16.c_str());
  EXPECT_TRUE(UTF16String(text->GetData()) == data);
  static UTF16Char extra_data[] = { 'D', 'A', 'T', 'A', 0 };
  text->AppendData(extra_data);
  EXPECT_STREQ("dataDATA", text->GetNodeValue());
  text->SetNodeValue("");
  text->AppendData(data);
  ASSERT_TRUE(UTF16String(text->GetData()) == data);

  EXPECT_EQ(DOM_NO_ERR, text->InsertData(0, extra_data));
  EXPECT_STREQ("DATAdata", text->GetNodeValue());
  EXPECT_EQ(DOM_NO_ERR, text->InsertData(8, extra_data));
  EXPECT_STREQ("DATAdataDATA", text->GetNodeValue());
  EXPECT_EQ(DOM_NO_ERR, text->InsertData(6, extra_data));
  EXPECT_STREQ("DATAdaDATAtaDATA", text->GetNodeValue());
  EXPECT_EQ(DOM_INDEX_SIZE_ERR, text->InsertData(17, extra_data));
  text->SetNodeValue("");
  EXPECT_EQ(DOM_NO_ERR, text->InsertData(0, data));
  ASSERT_TRUE(UTF16String(text->GetData()) == data);

  EXPECT_EQ(DOM_NO_ERR, text->DeleteData(0, 0));
  ASSERT_TRUE(UTF16String(text->GetData()) == data);
  EXPECT_EQ(DOM_NO_ERR, text->DeleteData(4, 0));
  ASSERT_TRUE(UTF16String(text->GetData()) == data);
  EXPECT_EQ(DOM_NO_ERR, text->DeleteData(0, 1));
  EXPECT_STREQ("ata", text->GetNodeValue());
  EXPECT_EQ(DOM_NO_ERR, text->DeleteData(1, 1));
  EXPECT_STREQ("aa", text->GetNodeValue());
  EXPECT_EQ(DOM_NO_ERR, text->DeleteData(0, 2));
  EXPECT_STREQ("", text->GetNodeValue());
  EXPECT_EQ(DOM_NO_ERR, text->DeleteData(0, 0));
  EXPECT_STREQ("", text->GetNodeValue());
  EXPECT_EQ(DOM_NO_ERR, text->InsertData(0, data));
  EXPECT_EQ(DOM_INDEX_SIZE_ERR, text->DeleteData(5, 0));
  EXPECT_EQ(DOM_NO_ERR, text->DeleteData(0, 5));
  EXPECT_STREQ("", text->GetNodeValue());
  EXPECT_EQ(DOM_NO_ERR, text->InsertData(0, data));
  ASSERT_TRUE(UTF16String(text->GetData()) == data);

  EXPECT_EQ(DOM_NO_ERR, text->ReplaceData(0, 0, extra_data));
  EXPECT_STREQ("DATAdata", text->GetNodeValue());
  EXPECT_EQ(DOM_NO_ERR, text->ReplaceData(6, 2, extra_data));
  EXPECT_STREQ("DATAdaDATA", text->GetNodeValue());
  EXPECT_EQ(DOM_NO_ERR, text->ReplaceData(6, 1, extra_data));
  EXPECT_STREQ("DATAdaDATAATA", text->GetNodeValue());
  EXPECT_EQ(DOM_INDEX_SIZE_ERR, text->ReplaceData(14, 0, extra_data));
  EXPECT_EQ(DOM_NO_ERR, text->ReplaceData(0, 14, extra_data));
  EXPECT_STREQ("DATA", text->GetNodeValue());
  text->SetNodeValue("");
  EXPECT_EQ(DOM_NO_ERR, text->ReplaceData(0, 0, data));
  ASSERT_TRUE(UTF16String(text->GetData()) == data);

  DOMTextInterface *text1 = doc->CreateTextNode(data);
  EXPECT_EQ(DOM_HIERARCHY_REQUEST_ERR, text->AppendChild(text1));
  delete text1;

  EXPECT_EQ(DOM_NO_ERR, text->SplitText(0, &text1));
  EXPECT_STREQ("", text->GetNodeValue());
  EXPECT_STREQ("data", text1->GetNodeValue());
  delete text;

  EXPECT_EQ(DOM_NO_ERR, text1->SplitText(4, &text));
  EXPECT_STREQ("", text->GetNodeValue());
  EXPECT_STREQ("data", text1->GetNodeValue());
  delete text;

  EXPECT_EQ(DOM_NO_ERR, text1->SplitText(2, &text));
  EXPECT_STREQ("ta", text->GetNodeValue());
  EXPECT_STREQ("da", text1->GetNodeValue());

  DOMElementInterface *root_ele;
  ASSERT_EQ(DOM_NO_ERR, doc->CreateElement("root", &root_ele));
  ASSERT_EQ(DOM_NO_ERR, doc->AppendChild(root_ele));
  ASSERT_EQ(DOM_NO_ERR, root_ele->AppendChild(text));
  ASSERT_EQ(DOM_NO_ERR, root_ele->AppendChild(text1));
  root_ele->Normalize();
  text = down_cast<DOMTextInterface *>(root_ele->GetFirstChild());
  ASSERT_TRUE(text->GetNextSibling() == NULL);
  EXPECT_STREQ("tada", text->GetNodeValue());
  EXPECT_EQ(DOM_INDEX_SIZE_ERR, text->SplitText(5, &text1));
  EXPECT_TRUE(text1 == NULL);
  EXPECT_EQ(DOM_NO_ERR, text->SplitText(2, &text1));
  EXPECT_TRUE(text1->GetParentNode() == root_ele);
  EXPECT_TRUE(text1->GetPreviousSibling() == text);
  EXPECT_STREQ("ta", text->GetNodeValue());
  EXPECT_STREQ("da", text1->GetNodeValue());
  DOMTextInterface *text2;
  EXPECT_EQ(DOM_NO_ERR, text->SplitText(1, &text2));
  EXPECT_TRUE(text2->GetParentNode() == root_ele);
  EXPECT_TRUE(text2->GetPreviousSibling() == text);
  EXPECT_TRUE(text2->GetNextSibling() == text1);
  EXPECT_STREQ("t", text->GetNodeValue());
  EXPECT_STREQ("a", text2->GetNodeValue());
  doc->Detach();
}

TEST(XMLDOM, TestDocumentFragmentAndTextContent) {
  DOMDocumentInterface *doc = CreateDOMDocument();
  doc->Attach();
  DOMElementInterface *root_ele;
  ASSERT_EQ(DOM_NO_ERR, doc->CreateElement("root", &root_ele));
  ASSERT_EQ(DOM_NO_ERR, doc->AppendChild(root_ele));

  DOMDocumentFragmentInterface *fragment = doc->CreateDocumentFragment();
  fragment->Attach();
  TestBlankNode(fragment);
  TestNullNodeValue(fragment);
  ASSERT_EQ(DOMNodeInterface::DOCUMENT_FRAGMENT_NODE, fragment->GetNodeType());
  ASSERT_EQ(DOM_NO_ERR, root_ele->AppendChild(fragment));
  EXPECT_TRUE(root_ele->GetFirstChild() == NULL);

  static UTF16Char data[] = { 'd', 'a', 't', 'a', 0 };
  fragment->SetTextContent("DATA");
  ASSERT_EQ(DOM_NO_ERR, fragment->AppendChild(doc->CreateTextNode(data)));
  ASSERT_STREQ("DATAdata", fragment->GetTextContent());
  ASSERT_EQ(DOM_NO_ERR, root_ele->AppendChild(fragment));
  TestBlankNode(fragment);
  ASSERT_STREQ("", fragment->GetTextContent());

  EXPECT_TRUE(root_ele->GetFirstChild());
  EXPECT_TRUE(root_ele->GetFirstChild()->GetNextSibling());
  EXPECT_TRUE(root_ele->GetFirstChild()->GetNextSibling()->GetNextSibling() ==
              NULL);
  EXPECT_STREQ("DATAdata", root_ele->GetTextContent());
  ASSERT_EQ(DOM_NO_ERR, root_ele->AppendChild(root_ele->CloneNode(true)));
  data[0] = 'E';
  ASSERT_EQ(DOM_NO_ERR, root_ele->AppendChild(doc->CreateCDATASection(data)));
  data[0] = 'F';
  ASSERT_EQ(DOM_NO_ERR, root_ele->AppendChild(doc->CreateComment(data)));
  EXPECT_STREQ("DATAdataDATAdataEata", root_ele->GetTextContent());

  root_ele->SetTextContent("NEW");
  EXPECT_STREQ("NEW", root_ele->GetTextContent());

  fragment->Detach();
  doc->Detach();
}

TEST(XMLDOM, Others) {
  DOMDocumentInterface *doc = CreateDOMDocument();
  doc->Attach();
  DOMElementInterface *root_ele;
  ASSERT_EQ(DOM_NO_ERR, doc->CreateElement("root", &root_ele));
  ASSERT_EQ(DOM_NO_ERR, doc->AppendChild(root_ele));

  ASSERT_TRUE(doc->GetDoctype() == NULL);
  DOMImplementationInterface *impl = doc->GetImplementation();
  ASSERT_TRUE(impl->HasFeature("XML", "1.0"));
  ASSERT_TRUE(impl->HasFeature("XML", NULL));
  ASSERT_FALSE(impl->HasFeature("XPATH", NULL));

  DOMCommentInterface *comment = doc->CreateComment(NULL);
  TestBlankNode(comment);
  ASSERT_EQ(DOMNodeInterface::COMMENT_NODE, comment->GetNodeType());
  ASSERT_EQ(DOM_NO_ERR, doc->AppendChild(comment));

  DOMCDATASectionInterface *cdata = doc->CreateCDATASection(NULL);
  TestBlankNode(cdata);
  ASSERT_EQ(DOMNodeInterface::CDATA_SECTION_NODE, cdata->GetNodeType());
  ASSERT_EQ(DOM_HIERARCHY_REQUEST_ERR, doc->AppendChild(cdata));
  ASSERT_EQ(DOM_NO_ERR, root_ele->AppendChild(cdata));

  DOMProcessingInstructionInterface *pi;
  ASSERT_EQ(DOM_NO_ERR, doc->CreateProcessingInstruction("pi", "value", &pi));
  TestBlankNode(pi);
  ASSERT_EQ(DOMNodeInterface::PROCESSING_INSTRUCTION_NODE, pi->GetNodeType());
  ASSERT_EQ(DOM_NO_ERR, doc->AppendChild(pi));
  ASSERT_EQ(DOM_NO_ERR, root_ele->AppendChild(pi));

  ASSERT_TRUE(doc->CloneNode(true) == NULL);

  doc->Detach();
}

int main(int argc, char **argv) {
  testing::ParseGUnitFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
