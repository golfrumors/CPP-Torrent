
namespace BitTorrent{
	class BEncoding{
		public:
			static constexpr char DictStart = 'd';
			static constexpr char DictEnd = 'e';
			static constexpr char ListStart = 'l';
			static constexpr char ListEnd = 'e';
			static constexpr char IntStart = 'i';
			static constexpr char IntEnd = 'e';
			static constexpr char ByteArrDivider = ':';

			static std::any Decode(const std::vector<char>& bytes);
	};
}

//public
std::any BitTorrent::BEncoding::Decode(const std::vector<char>& bytes){
	auto it = bytes.begin();
	return DecodeNextObj(it);
}

//public
std::any BitTorrent::BEncoding::DecodeFile(const std::string& path) {
	std::vector<char> bytes;
	std::ifstream file(path, std::io::binary);

	if(file){
		file.seekg(0, std::ios::end);
		bytes.resize(file.tellg());
		file.seekg(0, std::ios::beg);
		file.read(bytes.data(), bytes.size());
	}
	file.close();

	return Decode(bytes);
}

//private
static std::any DecodeNextObj(std::vector<char>::const_iterator& it){
	if (*it == DictStart){
		return DecodeDict(it);
	}

	if (*it == ListStart){
		return DecodeList(it);
	}

	if (*it == IntStart){
		return DecodeInt(it);
	}

	return DecodeByteArr(it);
}

//private
std::unordered_map<std::string, std::any> DecodeDictionary(std::vector<byte>::const_iterator& it){
	std::unordered_map<std::string, std::any> dict;
	std::vector<std::string> keys;

	while(it != end(bytes)) {
		if(*it == DictEnd) {
			break;
		}
		
		std::vector<byte> keyBytes = DecodeByteArr(it);
		std::string key(keyBytes.begin(), keyBytes.end());
		std::advance(it, 1);
		std::any val = DecodeNextObj(it);

		keys.emplace_back(key);
		dict.emplace(std::move(key), std::move(val));
	}

	auto sortedKeys = keys;
	std::sort(sortedKeys.begin(), sortedKeys.end(), [](const auto& lhs, const auto& rhs) {
			return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
	});

	if(!std::equal(keys.begin(), keys.end(), sortedKeys.begin(), sortedKeys.end())) {
		throw std::exception("Erorr loading dict: Keys unsorted");
	}

	return dict;
}

//private
std::vector<std::byte> DecodeList(std::list<std::byte>::const_iterator& it){
	std::vector<std::byte> list;

	while(it != end(bytes)) {
		if(*it == ListEnd) {
			break;
		}

		std::any val = DecodeNextObj(it);
		list.emplace_back(std::move(val));
	}

	return list;
}

//private
std::vector<byte> DecodeByteArr(std::vector<byte>::iterator& it){
	std::vector<byte> lenBytes;

	//go until the divider
	while(*it != ByteArrDivider) {
		lenBytes.push_back(*it);
		std::advance(it, 1);
	}

	std::advance(it, 1);

	std::string lenStr(lenBytes.begin(), lenBytes.end());

	int len;

	if(!TryParse(lenStr, len)) {
		throw std::exception("Error loading byte array: Invalid length");
	}

	//read the real byte arr
	std::vector<byte> bytes(len);

	for(int i = 0; i < len; i++) {
		bytes[i] = *it;
		std::advance(it, 1);
	}

	return bytes;
}
