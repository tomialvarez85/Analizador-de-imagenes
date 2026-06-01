#ifndef CONFIG_H
#define CONFIG_H

namespace AppConfig {

constexpr const char kApiBaseUrl[] = "https://185.194.217.220/api";

inline const char *apiBaseUrl()
{
    return kApiBaseUrl;
}

} // namespace AppConfig

#endif // CONFIG_H
