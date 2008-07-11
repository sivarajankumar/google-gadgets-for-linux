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

#include "gadgets_metadata.h"

#include <cstdlib>
#include <time.h>
#include <ggadget/file_manager_factory.h>
#include <ggadget/scriptable_holder.h>
#include <ggadget/slot.h>
#include <ggadget/xml_http_request_interface.h>
#include <ggadget/xml_parser_interface.h>

namespace ggadget {
namespace google {

static const char kDefaultQueryDate[] = "01011980";
static const char kQueryDateFormat[] = "%m%d%Y";

static const char *kMonthNames[] = {
  "January", "February", "March", "April", "May", "June",
  "July", "August", "September", "October", "November", "December"
};

class GadgetsMetadata::Impl {
 public:
  Impl()
      : parser_(GetXMLParser()),
        file_manager_(GetGlobalFileManager()),
        latest_plugin_time_(0),
        full_download_(false),
        on_update_done_(NULL) {
    ASSERT(parser_);
    ASSERT(file_manager_);
    Init();
  }

  void Init() {
    std::string contents;
    if (file_manager_->ReadFile(kPluginsXMLLocation, &contents))
      ParsePluginsXML(contents, true);
  }

  ~Impl() {
    if (request_.Get())
      request_.Get()->Abort();
  }

  void EnsureInitialized() {
    if (plugins_.empty())
      Init();
  }

  void FreeMemory() {
    if (!request_.Get())
      plugins_.clear();
  }

  // Get a value from a XPath map.
  static std::string GetValue(const StringMap &table,
                              const std::string &key) {
    StringMap::const_iterator it = table.find(key);
    return it == table.end() ? std::string() : it->second;
  }

  static uint64_t ParsePluginUpdatedDate(const StringMap &table,
                                         const std::string &plugin_key) {
    std::string updated_date_str = GetValue(table,
                                            plugin_key + "@updated_date");
    if (updated_date_str.empty())
      updated_date_str = GetValue(table, plugin_key + "@creation_date");

    return updated_date_str.empty() ? 0 : ParseDate(updated_date_str.c_str());
  }

  // In the incremental plugins.xml, plugins are matched with the uuid for
  // desktop gadgets, and the download_url for iGoogle gadgets.
  static std::string GetPluginID(const StringMap &table,
                                 const std::string &plugin_key) {
    std::string id = ToUpper(GetValue(table, plugin_key + "@guid"));
    if (id.empty())
      id = GetValue(table, plugin_key + "@download_url");
    return id;
  }

  // Parse date string in plugins.xml, in format like "November 10, 2007".
  // strptime() is not portable enough, so we write our own code instead.
  static uint64_t ParseDate(const std::string &date_str) {
    std::string year_str, month_str, day_str;
    if (!SplitString(date_str, " ", &month_str, &day_str) ||
        !SplitString(day_str, " ", &day_str, &year_str) ||
        month_str.size() < 3)
      return 0;

    struct tm time;
    memset(&time, 0, sizeof(time));
    time.tm_year = static_cast<int>(strtol(year_str.c_str(), NULL, 10)) - 1900;
    // The ',' at the end of day_str will be automatically ignored.
    time.tm_mday = static_cast<int>(strtol(day_str.c_str(), NULL, 10));
    time.tm_mon = -1;
    for (size_t i = 0; i < arraysize(kMonthNames); i++) {
      if (month_str == kMonthNames[i]) {
        time.tm_mon = static_cast<int>(i);
        break;
      }
    }
    if (time.tm_mon == -1)
      return 0;

    time_t local_time = mktime(&time);
    time_t local_time_as_gm = mktime(gmtime(&local_time));
    // Now local_time_as_gm - local_time is the time difference between gmt
    // and local time.
    if (local_time < local_time_as_gm - local_time)
      return 0;
    return (local_time - (local_time_as_gm - local_time)) * UINT64_C(1000);
  }

  bool SavePluginsXMLFile() {
    std::string contents("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                         "<plugins>\n");
    for (GadgetInfoMap::const_iterator it = plugins_.begin();
         it != plugins_.end(); ++ it) {
      const GadgetInfo &info = it->second;
      contents += " <plugin";
      for (StringMap::const_iterator attr_it = info.attributes.begin();
           attr_it != info.attributes.end(); ++attr_it) {
        contents += " ";
        contents += attr_it->first;
        contents += "=\"";
        contents += parser_->EncodeXMLString(attr_it->second.c_str());
        contents += "\"";
      }
      if (info.titles.empty() && info.descriptions.empty()) {
        contents += "/>\n";
      } else {
        contents += ">\n";
        for (StringMap::const_iterator title_it = info.titles.begin();
             title_it != info.titles.end(); ++title_it) {
          contents += "  <title locale=\"";
          contents += parser_->EncodeXMLString(title_it->first.c_str());
          contents += "\">";
          contents += parser_->EncodeXMLString(title_it->second.c_str());
          contents += "</title>\n";
        }
        for (StringMap::const_iterator desc_it =
                 info.descriptions.begin();
             desc_it != info.descriptions.end(); ++desc_it) {
          contents += "  <description locale=\"";
          contents += parser_->EncodeXMLString(desc_it->first.c_str());
          contents += "\">";
          contents += parser_->EncodeXMLString(desc_it->second.c_str());
          contents += "</description>\n";
        }
        contents += " </plugin>\n";
      }
    }
    contents += "</plugins>\n";
    return file_manager_->WriteFile(kPluginsXMLLocation, contents, true);
  }

  bool ParsePluginsXML(const std::string &contents, bool full_update) {
    if (!full_update)
      EnsureInitialized();

    StringMap new_plugins;
    if (!parser_->ParseXMLIntoXPathMap(contents, NULL, kPluginsXMLLocation,
                                       "plugins", NULL, NULL, &new_plugins))
      return false;

    GadgetInfoMap temp_plugins;
    // Update the latest gadget time and plugin index.
    latest_plugin_time_ = 0;
    StringMap::const_iterator it = new_plugins.begin();
    while (it != new_plugins.end()) {
      const std::string &plugin_key = it->first;
      ++it;
      if (!SimpleMatchXPath(plugin_key.c_str(), "plugin"))
        continue;

      // Don't be confused about this id and the id attribute. This id is
      // uuid for desktop gadgets and download_url for iGoogle gadgets, and
      // is used thoughout our system to identify gadgets. The id attribute
      // in the plugins.xml is not used by other parts of this system.
      std::string id = GetPluginID(new_plugins, plugin_key);
      if (id.empty())
        continue;

      // The id attribute is used only here to detect if the plugin record is
      // a full record.
      if (GetValue(new_plugins, plugin_key + "@id").empty()) {
        GadgetInfoMap::iterator org_info_it = plugins_.find(id);
        if (full_update) {
          LOG("Partial record found during full update: %s", id.c_str());
          return false;
        } else if (org_info_it == plugins_.end()) {
          LOG("Can't find orignal plugin info when updating %s", id.c_str());
          // This may be caused by corrupted cached plugins.xml file.
          return false;
        } else {
          // This is a partial record which contains only an optional 'rank'
          // attribute.
          GadgetInfo *info = &temp_plugins[id];
          *info = org_info_it->second;
          std::string rank = GetValue(new_plugins, plugin_key + "@rank");
          if (!rank.empty())
            info->attributes["rank"] = rank;
        }
        continue;
      }

      // Otherwise, this is a full record.
      GadgetInfo *info = &temp_plugins[id];
      info->id = id;
      info->updated_date = ParsePluginUpdatedDate(new_plugins, plugin_key);
      if (info->updated_date > latest_plugin_time_)
        latest_plugin_time_ = info->updated_date;

      // Enumerate all attributes and sub elements of this plugin.
      while (it != new_plugins.end()) {
        const std::string &key = it->first;
        if (GadgetStrNCmp(key.c_str(), plugin_key.c_str(),
                          plugin_key.size()) != 0) {
          // Finished parsing data of the current gadget.
          break;
        }

        char next_char = key[plugin_key.size()];
        if (next_char == '@') {
          info->attributes[key.substr(plugin_key.size() + 1)] = it->second;
        } else if (next_char == '/') {
          if (SimpleMatchXPath(key.c_str(), "plugin/title")) {
            std::string locale =
                ToLower(GetValue(new_plugins, key + "@locale"));
            if (locale.empty())
              LOG("Missing 'locale' attribute in <title>");
            else
              info->titles[locale] = it->second;
          } else if (SimpleMatchXPath(key.c_str(), "plugin/description")) {
            std::string locale =
                ToLower(GetValue(new_plugins, key + "@locale"));
            if (locale.empty())
              LOG("Missing 'locale' attribute in <description>");
            else
              info->descriptions[locale] = it->second;
          }
        } else {
          // Finished parsing data of the current gadget.
          break;
        }
        ++it;
      }
    }

    plugins_.swap(temp_plugins);
    return true;
  }

  std::string GetQueryDate() {
    if (!full_download_ && latest_plugin_time_ > 0) {
      time_t time0 = static_cast<time_t>(latest_plugin_time_ / 1000);
      struct tm *time = gmtime(&time0);
      char buf[9];
      strftime(buf, sizeof(buf), kQueryDateFormat, time);
      return std::string(buf);
    } else {
      return kDefaultQueryDate;
    }
  }

  void OnRequestReadyStateChange() {
    XMLHttpRequestInterface *request = request_.Get();
    if (request && request->GetReadyState() == XMLHttpRequestInterface::DONE) {
      unsigned short status;
      bool request_success = false, parsing_success = false;
      if (request->GetStatus(&status) == XMLHttpRequestInterface::NO_ERR &&
          status == 200) {
        // The request finished successfully. Use GetResponseBody() instead of
        // GetResponseText() because it's more lightweight.
        std::string response_body;
        if (request->GetResponseBody(&response_body) ==
                XMLHttpRequestInterface::NO_ERR) {
          request_success = true;
          parsing_success = ParsePluginsXML(response_body, full_download_);
          if (parsing_success)
            SavePluginsXMLFile();
        }
      }

      if (on_update_done_) {
        (*on_update_done_)(request_success, parsing_success);
        delete on_update_done_;
        on_update_done_ = NULL;
      }
      // Release the reference.
      request_.Reset(NULL);
    }
  }

  void UpdateFromServer(bool full_download, XMLHttpRequestInterface *request,
                        Slot2<void, bool, bool> *on_done) {
    ASSERT(request);
    ASSERT(request->GetReadyState() == XMLHttpRequestInterface::UNSENT);

    // TODO: Check disk free space.
    if (request_.Get())
      request_.Get()->Abort();
    full_download_ = full_download;
    if (on_update_done_)
      delete on_update_done_;
    on_update_done_ = on_done;

    std::string request_url(kPluginsXMLRequestPrefix);
    request_url += "&diff_from_date=";
    request_url += GetQueryDate();

    request_.Reset(request);
    request->ConnectOnReadyStateChange(
        NewSlot(this, &Impl::OnRequestReadyStateChange));
    if (request->Open("GET", request_url.c_str(), true, NULL, NULL) ==
        XMLHttpRequestInterface::NO_ERR) {
      request->Send(NULL);
    }
  }

  GadgetInfoMap *GetAllGadgetInfo() {
    EnsureInitialized();
    return &plugins_;
  }

  XMLParserInterface *parser_;
  FileManagerInterface *file_manager_;
  ScriptableHolder<XMLHttpRequestInterface> request_;
  uint64_t latest_plugin_time_;
  bool full_download_;
  GadgetInfoMap plugins_;
  Slot2<void, bool, bool> *on_update_done_;
};

GadgetsMetadata::GadgetsMetadata()
    : impl_(new Impl()) {
}

GadgetsMetadata::~GadgetsMetadata() {
  delete impl_;
}

void GadgetsMetadata::Init() {
  impl_->Init();
}

void GadgetsMetadata::FreeMemory() {
  DLOG("GadgetsMetadata free memory");
  impl_->FreeMemory();
}

void GadgetsMetadata::UpdateFromServer(bool full_download,
                                       XMLHttpRequestInterface *request,
                                       Slot2<void, bool, bool> *on_done) {
  impl_->UpdateFromServer(full_download, request, on_done);
}

GadgetInfoMap *GadgetsMetadata::GetAllGadgetInfo() {
  return impl_->GetAllGadgetInfo();
}

const GadgetInfoMap *GadgetsMetadata::GetAllGadgetInfo() const {
  return impl_->GetAllGadgetInfo();
}

} // namespace google
} // namespace ggadget