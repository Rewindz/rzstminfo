#pragma once
#include <string>
#include <future>


namespace rz
{

  enum class SteamIDType
  {
    STEAM_ID_64,
    STEAM_ID_3
  };

  class SteamAccount
  {
  public:

    SteamAccount(const SteamAccount&) = default;

    SteamAccount(const std::string& _id, SteamIDType _type);

    const std::string& GetId(SteamIDType _type = SteamIDType::STEAM_ID_64) const;
    const std::string& GetUName() const;
    
    /*
      Fetch the Steam user's name.
      Non-blocking.
      Returns a boolean of success status.
      If fetching for multiple accounts, it is ideal to do this in a worker thread
      and sleep the thread between fetches.
    */
    std::future<bool> FetchUserNameAsync();
    
    /* 
      Fetch the Steam user's name.
      This is blocking and not preferred.
      Returns a boolean of success status.
    */
    bool FetchUserName();

  private:
    
    std::string id64, id3;
    std::string uname;
    
    bool idConv(const std::string& _id, std::string& _res, SteamIDType _fromType);
    bool getUserName();
    
  };

} // namespace rz