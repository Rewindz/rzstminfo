#include "SteamAccount.hpp"

#include <string>
#include <charconv>
#include <system_error>
#include <format>
#include <future>
#include <thread>
#include <cstdlib>
#include <chrono>
#include <exception>
#include <fstream>
#include <ranges>
#include <utility>
#include <iostream>

#include <httplib.h>
#include <vdf_parser.hpp>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

// Steam Base ID64
constexpr uint64_t STEAM_MAGIC_NUMBER = 76561197960265728;

namespace rz
{

  bool AccountInfo::HasId() const
  {
    return id3.has_value() || id64.has_value();
  }

  std::optional<std::filesystem::path> getSteamPath()
  {
      static std::optional<std::filesystem::path> path = []() -> std::optional<std::filesystem::path> {
          #ifdef __linux__
              const char *home = std::getenv("HOME");
              if(!home)
                  return std::nullopt;

              std::filesystem::path homePath(home);    
              std::vector<std::filesystem::path> supportedPaths = {
                homePath / ".steam" / "steam",
                homePath / ".local" / "share" / "Steam",
                homePath / ".var" / "app" / "com.valvesoftware.Steam" / ".steam" / "steam",
                homePath / "snap" / "steam" / "common" / ".steam" / "steam"
              };

              for(const auto& path : supportedPaths){
                if(std::filesystem::exists(path))
                  return path;
              }
              return std::nullopt; 
          #elifdef _WIN32
              char steamPath[MAX_PATH];
              DWORD bufferSize = sizeof(steamPath);
              LSTATUS status = RegGetValueA(HKEY_CURRENT_USER, "Software\\Valve\\Steam", "SteamPath", RRF_RT_REG_SZ, nullptr, steamPath, &bufferSize);
              if (status == ERROR_SUCCESS) {
                std::filesystem::path path(steamPath);
                if(std::filesystem::exists(path))
                  return path;
              }
              return std::nullopt;
          #elifdef __APPLE__
              const char* home = std::getenv("HOME");
              if(!home)
                return std::nullopt;
              std::filesystem::path path = std::filesystem::path(homePath) / "Library" / "Application Support" / "Steam";
              if(std::filesystem::exists(path))
                return path;
              return std::nullopt;
          #endif

          return std::nullopt;
      }();
      return path;
  }

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

    auto optSteamPath = getSteamPath();
    if(optSteamPath)
      userdataPath = optSteamPath.value() / "userdata" / id3;
    else
      userdataPath = std::filesystem::path();
    
  }

  SteamAccount::SteamAccount(const AccountInfo& _accInfo)
  {
    id3 = _accInfo.id3.value_or("");
    id64 = _accInfo.id64.value_or("");
    SetUName(_accInfo.uname.value_or(""));

    if(id3.empty() && id64.empty()){
      throw std::invalid_argument("Cannot create SteamAccount: Both id3 and id64 are empty.");
    }

    if(id3.empty()){
      idConv(id64, id3, SteamIDType::STEAM_ID_64);
    } else if (id64.empty()){
      idConv(id3, id64, SteamIDType::STEAM_ID_3);
    }

    auto optSteamPath = getSteamPath();
    if(optSteamPath)
      userdataPath = optSteamPath.value() / "userdata" / id3;
    else
      userdataPath = std::filesystem::path();
    
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

  std::string SteamAccount::GetUName() const
  {
    std::shared_lock lock(uname_mutex);
    return uname;
  }

  const std::filesystem::path& SteamAccount::GetUserdataPath() const
  {
    return userdataPath;
  }

  bool SteamAccount::HasUserdataGameDir(const std::string& _gameId) const
  {
    auto path = userdataPath / _gameId;
    return std::filesystem::exists(path) && std::filesystem::is_directory(path);
  }

  std::optional<std::filesystem::path> SteamAccount::GetUserdataGameDir(const std::string& _gameId) const
  {
    auto path = userdataPath / _gameId;
    if(std::filesystem::exists(path) && std::filesystem::is_directory(path))
      return path;
    else
      return std::nullopt;
  }

  void SteamAccount::SetUName(const std::string& _uname)
  {
    std::unique_lock lock(uname_mutex);
    uname = _uname;
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
              std::string fetchedName =  body.substr(startPos, endPos - startPos);
              if(fetchedName.starts_with("<![CDATA[") && fetchedName.ends_with("]]>")){
                fetchedName = fetchedName.substr(9, fetchedName.length() - 12);
              }
              {
                std::unique_lock lock(uname_mutex);
                uname = fetchedName;
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
    auto [ptr, ec] = std::from_chars(_id.data(), _id.data() + _id.size(), tmp);
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

  void SteamAccountsManager::AddAccounts(const AccountInfoArray& _accounts, bool _fetchNames)
  {

    activeBatches.erase(std::remove_if(activeBatches.begin(), activeBatches.end(), [](const std::future<void>& f){
      return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }), activeBatches.end());
    
    std::vector<SteamAccount*> batch;
    for(const auto& acc : _accounts)
    {
      if(!acc.HasId())
      continue;
      try{
        accounts.push_back(std::make_unique<SteamAccount>(acc));
        if(_fetchNames && !acc.uname){
          batch.push_back(accounts.back().get());
        }
      } catch (const std::invalid_argument& e) {
        std::cerr << std::format("[rzstminfo] Failed to add account!\n{}\n", e.what());
      }
    }
    
    if(_fetchNames && !batch.empty())
    {

      auto worker = [batch]() -> void{
        for(auto* acc : batch){
          auto future = acc->FetchUserNameAsync();
          future.wait();
          std::this_thread::sleep_for(std::chrono::seconds(1));
        }
      };

      activeBatches.push_back(std::async(std::launch::async, worker));

    }

  }

  SteamAccount* SteamAccountsManager::AddAccount(std::unique_ptr<SteamAccount> _account)
  {
    accounts.push_back(std::move(_account));
    return accounts.back().get();
  }

  void SteamAccountsManager::AddAccountsFromLogin()
  {
    auto steamPathOpt = getSteamPath();
    if(steamPathOpt){
      AccountInfoArray infoArr;
      
      auto loginPath = steamPathOpt.value() / "config" / "loginusers.vdf";
      std::ifstream loginStream(loginPath);
      if(loginStream.is_open()){
        auto root = tyti::vdf::read(loginStream);
        loginStream.close();
        for(const auto& [id64, child] : root.childs)
        {
            const auto& uname = child->attribs["PersonaName"];
            AccountInfo info{};
            info.id64 = id64;
            info.uname = uname;
            infoArr.push_back(info); 
        }
      }
      AddAccounts(infoArr);
    }
  }

  std::optional<SteamAccount*> SteamAccountsManager::GetAccountFrom64(const std::string& _id64)
  {
    auto res = std::ranges::find_if(accounts, [&_id64](const std::unique_ptr<SteamAccount>& acc){
      return acc->GetId(SteamIDType::STEAM_ID_64) == _id64;
    });
    if(res == accounts.end())
      return std::nullopt; 
    return (*res).get();
  }

  std::vector<SteamAccount*> SteamAccountsManager::GetAccounts() const
  {
    std::vector<SteamAccount*> res;
    res.reserve(accounts.size());
    for(const auto& acc : accounts)
    {
      res.push_back(acc.get());
    }
    return res;
  }

  std::unordered_map<std::string, SteamAccount*> SteamAccountsManager::GetAccountsAsMap() const
  {
    std::unordered_map<std::string, SteamAccount*> res;
    res.reserve(accounts.size());
    for(const auto& acc : accounts){
      res.emplace(acc->GetId(SteamIDType::STEAM_ID_64), acc.get());
    }
    return res;
  }

  SteamAccountsManager& SteamAccountsManager::GetInstance()
  {
    static SteamAccountsManager instance;
    return instance;
  }

} // namespace rz