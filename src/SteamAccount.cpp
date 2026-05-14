#include "SteamAccount.hpp"

#include <string>
#include <charconv>
#include <system_error>
#include <format>
#include <future>
#include <thread>

#include <httplib.h>

constexpr uint64_t STEAM_MAGIC_NUMBER = 76561197960265728;

namespace rz
{

  SteamAccount::SteamAccount(const std::string &_id, SteamIDType _type)
  {
    switch (_type)
    {
      case SteamIDType::STEAM_ID_3:
      {
        id3 = _id;
        idConv(id3, id64, SteamIDType::STEAM_ID_3);
        break;
      }
      case SteamIDType::STEAM_ID_64:
      {
        id64 = _id;
        idConv(id64, id3, SteamIDType::STEAM_ID_64);
        break;
      }
    }
  }

  const std::string& SteamAccount::GetId(SteamIDType _type) const
  {
    switch(_type)
    {
      default:
      case SteamIDType::STEAM_ID_64:
        return id64;
      case SteamIDType::STEAM_ID_3:
        return id3;
    }
  }

  bool SteamAccount::getUserName()
  {
    if(id64.empty())
      return false;

    httplib::Client cli("https://steamcommunity.com");
    cli.set_default_headers({
        {"User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36"},
        {"Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8"},
        {"Accept-Language", "en-US,en;q=0.5"}
    });
    cli.set_follow_location(true);

    auto response = cli.Get(std::format("/profiles/{}?xml=1", id64));
    if(response && response->status == httplib::StatusCode::OK_200){
      std::string body = response->body;
      std::string startTag = "<steamID>";
      std::string endTag = "</steamID>";
      size_t startPos = body.find(startTag);
      if(startPos != std::string::npos){
          startPos += startTag.length();
          size_t endPos = body.find(endTag, startPos);
          if(endPos != std::string::npos){
              uname = body.substr(startPos, endPos - startPos);
              if(uname.starts_with("<![CDATA[") && uname.ends_with("]]>")){
                  uname = uname.substr(9, uname.length() - 12);
              }
              return true;
          } else return false;
      } else return false;
    }

    return false;
  }

  bool SteamAccount::FetchUserName()
  {
    return getUserName();
  }

  std::future<bool> SteamAccount::FetchUserNameAsync()
  {
    return std::async(std::launch::async, [this]() -> bool {
      return getUserName();
    });
  }

  bool SteamAccount::idConv(const std::string& _id, std::string& _res, SteamIDType _fromType)
  {
    if(_id.empty())
      return false;

    uint64_t tmp = 0;
    auto [ptr, ec] = std::from_chars(id3.data(), id3.data() + id3.size(), tmp);
    if(ec != std::errc())
      return false;


    switch(_fromType){
      case SteamIDType::STEAM_ID_64:
      {
        _res = std::to_string( tmp - STEAM_MAGIC_NUMBER );
        break;
      }
      case SteamIDType::STEAM_ID_3:
      {
        _res = std::to_string( tmp + STEAM_MAGIC_NUMBER );
        break;
      }
    }
    return true;
  }

} // namespace rz