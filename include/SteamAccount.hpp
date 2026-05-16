#pragma once
#include <string>
#include <future>
#include <filesystem>
#include <optional>
#include <vector>
#include <memory>

namespace rz
{

  enum class SteamIDType
  {
    STEAM_ID_64,
    STEAM_ID_3
  };

  /**
   * @struct AccountFillInfo
   * 
   * @brief Used to hold account info for filling SteamAccountsManager with accounts.
   * 
   * @note Use this struct to fill SteamAccountsManager with local saved data 
   * (such as a .json with account information).
   */
  struct AccountInfo
  {
    std::optional<std::string> id64 = std::nullopt;
    std::optional<std::string> id3 = std::nullopt;
    std::optional<std::string> uname = std::nullopt;

    bool HasId() const;

    AccountInfo() = default;
  };

  using AccountInfoArray = std::vector<AccountInfo>;

  std::optional<std::filesystem::path> getSteamPath();

  class SteamAccount
  {
  public:

    SteamAccount(const SteamAccount&) = default;

    SteamAccount(const std::string& _id, SteamIDType _type);

    SteamAccount(const AccountInfo& _accInfo);

    const std::string& GetId(SteamIDType _type = SteamIDType::STEAM_ID_64) const;
    const std::string& GetUName() const;
    const std::filesystem::path& GetUserdataPath() const;

    /**
     * @brief Set the Steam user's name
     * 
     * @note Use this function if fetching is not required (you have it saved somewhere).
     * 
     * @param _uname The string to set the username to.
     */
    void SetUName(const std::string& _uname);

    bool HasUserdataGameDir(const std::string& _gameId) const;

    /**
     * @brief Get the userdata directory for a given game id.
     * 
     * @param _gameId The game's id you want to get the directory for.
     * 
     * @return std::nullopt if the path does not exist or is not a directory,
     * returns the filepath otherwise.
     */
    std::optional<std::filesystem::path> GetUserdataGameDir(const std::string& _gameId) const;

    
    
    /**
     * @brief Fetches the Steam user's name asynchronously.
     * 
     * This function is non-blocking. 
     * 
     * @note If fetching for multiple accounts, it is ideal to do this in a worker 
     * thread and sleep the thread between fetches.
     * 
     * @return std::future<bool> representing the success status of the fetch operation.
     */
    std::future<bool> FetchUserNameAsync();
    
    /**
     * @brief Fetches the Steam user's name synchronously.
     * 
     * @warning This is a blocking call and is not preferred. Consider using 
     * FetchUserNameAsync() instead.
     * 
     * @return A boolean indicating the success status. True if successful, false otherwise.
     */
    bool FetchUserName();

  private:
    
    std::string id64, id3;
    std::string uname;
    std::filesystem::path userdataPath;
    
    bool idConv(const std::string& _id, std::string& _res, SteamIDType _fromType);
    bool getUserName();
    
  };

  /**
   * @class SteamAccountsManager
   * 
   * @brief Gets and manages all accounts on the system.
   * 
   * @note Singleton class
   */
  class SteamAccountsManager
  {
  public:

    SteamAccountsManager(SteamAccountsManager&) = delete;
    SteamAccountsManager& operator=(SteamAccountsManager&) = delete;
    
    SteamAccountsManager(SteamAccountsManager&&) = delete;
    SteamAccountsManager& operator=(SteamAccountsManager&&) = delete;

    void AddAccounts(const AccountInfoArray& _accounts, bool _fetchNames = false);

    /**
     * @brief Add accounts from the ${STEAMPATH}/config/loginusers.vdf file created by steam.
     */
    void AddAccountsFromLogin();

    /**
     * @return The singleton instance of SteamAccountsManager
     */
    static SteamAccountsManager& GetInstance();

  private:

    std::vector<std::unique_ptr<SteamAccount>> accounts;
    std::vector<std::future<void>> activeBatches;

    SteamAccountsManager() = default;

  };

} // namespace rz