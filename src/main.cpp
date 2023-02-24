#include <iostream>
#include <bitset>
#include <map>
#include <cxxopts/cxxopts.hpp>

#include "../include/TorrentClient.h"

int main(int argc, const char* argv[]) {
	cxxopts::Options options("TorrentClient", "Multi-threaded torrent client in C++");

	options.set_width(80).set_tab_expansion().add_options()
		("h,help", "Print help")
		("t,torrent-file", "Path to the torrent file", cxxopts::value<std::string>())
		("o,output-dir", "The output directory for the downloaded torrent", cxxopts::value<std::string>())
		("l,logging", "Enable logging", cxxopts::value<bool>()->default_value("false"))
		("n, thread-num", "Number of threads to use", cxxopts::value<int>()->default_value("5"))
		("f,log-file", "Path to the log file", cxxopts::value<std::string>()->default_value("../logs/client.log"))
		;


	try {
		auto parseOpts = options.parse(argc, argv);
		if(parseOpts.count("help")) {
			std::cout << options.help() << std::endl;
			return 0;
		}

		int threadNums = parseOpts["thread-num"].as<int>();
		bool enableLogging = parseOpts["logging"].as<bool>();
		std::string logFile = parseOpts["log-file"].as<std::string>();

		if(!parseOpts.count("torrent-file"))
			throw std::invalid_argument("Torrent file not specified");

		if(!parseOpts.count("output-dir")) 
			throw std::invalid_argument("Output directory not specified");
	
		std::string torrentFile = parseOpts["torrent-file"].as<std::string>();
		std::string outputDir = parseOpts["output-dir"].as<std::string>();
		TorrentClient client(threadNums, enableLogging, logFile);
		client.downloadFile(torrentFile, outputDir);
	} catch (std::exception& e) {
		std::cout << "Error parsing options: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
