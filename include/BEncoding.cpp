#include "BEncoding.h"

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

	class BEncodeValue {
		public:
			std::variant<std::vector<BEncodeValue>, std::map<std::string, BEncodeValue>, std::string, int> value;
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

long long DecodeInt(std::list<unsigned char>::iterator &it){
	std::list<unsigned char> bytes;

	//pull bytes until flag
	while(*it != IntEnd) {
		bytes.push_back(*it);
		it++;
	}

	std::string numAsString;
	for(unsigned char byte : bytes){
		numAsString += byte;
	}

	return std::stoll(numAsString);
}

//Encode
std::vector<unsigned char> Encode(const Object& obj) {
	std::vector<unsigned char> buffer;
	EncodeNextObj(buffer, obj);
	
	return buffer;
}

void EncodeNextObj(std::vector<byte>& buffer, connst Object& obj) {
	if(auto byteArr = dynamic_cast<const vector<byte> *>(&obj)) {
		EncodeByteArr(buffer, *byteArr);
	} else if (auto str = dynamic_cast<const string *>(&obj)) {
		EncodeStr(buffer, *str);
	} else if (auto num = dynamic_cast<const long *>(&obj)) {
		EncodeInt(buffer, *num);
	} else if (const list<Object> *list = dynamic_cast<const list<Object> *>(&obj)) {
		EncodeList(buffer, *list);
	} else if (const map<string, object> *dict = dynamic_cast<const map<string, Object> *>(&obj)) {
		EncodeDict(buffer, *dict);
	} else {
		throw std::exception("Unable to encode the type " + typeid(obj).name());
	}
}

//public
void EncodeToFile(const std::vector<uint8_t>& encoded, const std::string& path) {
	std::ofstream file(path, std::ios::binary);
	file.write(reinterpret_cast<const char*>(encoded.data()), encoded.size());
}

//private
void EncodeByteArr(std:vector<uint8_t>& buffer, const std::vector<uint8_t>& body) {
	std::string lenStr = std::to_string(body.size());
	std::vector<uint8_t> lenBytes(lenStr.begin(), lenStr.end());

	buffer.insert(buffer.end(), lenBytes.begin(), lenBytes.end());
	buffer.push_back(ByteArrDivider);
	buffer.insert(buffer.end(), body.begin(), body.end());
}

//private
void EncodeStr(vector<unsigned char>& buffer, const string& input) {
	vector<unsigned char> input_bytes = vector<unsigned char>(input.begin(), input.end());
	EncodeByteArr(buffer, input_bytes);
}

//private
void EncodeInt(vector<unsigned char>& buffer, long long input) {
	buffer.push_back(IntStart);
	string inputStr = to_string(input);
	vector<unsigned char> inputBytes = vector<unsigned char>(inputStr.begin(), inputStr.end());
	buffer.insert(buffer.end(), inputBytes.begin(), inputBytes.end());
	buffer.push_back(IntEnd);
}

//private
void EncodeList(std::vector<uint8_t>& buffer, const std::vector<std::variant<std::string, long, std::vector<uint8_t>, std::map<std::string, std::variant<std::string, long, std::vector<uint8_t>, std::map<std::string, std::variant<std::string, long, std::vector<uint8_t>, std::map<std::string, std::variant<std::string, long, std::vector<uint8_t>, std::map<std::string, std::variant<std::string, long, std::vector<uint8_t>>>>>>>>>& input)
{
    buffer.push_back(ListStart);
    for (const auto& item : input)
        EncodeNextObject(buffer, item);
    buffer.push_back(ListEnd);
}

//private
void EncodeDict(std::vector<unsigned char>& buffer, std::map<std::string, std::shared_ptr<BEncodeValue>> input) {
	buffer.push_back(DictStart);

	std::vector<std::string> sortedKeys;
	for(const auto& item : input) {
		sortedKeys.push_back(item.first);
	}
	sort(sortedKeys.begin(), sortedKeys.end(), [](const std::string& a, const std::string& b) {
		return a < b;
	});

	for (const auto& key : sortedKeys) {
		EncodeStr(buffer, key);
		EncodeNextObj(buffer, input[key]);
	}

	buffer.push_back(DictEnd);
}

//public
template<typename T>
std::string GetFormattedString(const T& obj, int depth = 0) {
	std::string output = "";
	if constexpr (std::is_same<T,std::vector<uint8_t>::value) {
		output += GetFormattedString(obj);
	} else if constexpr (std::is_integral_v<T>) {
		output += GetFormattedString(obj);
	} else if constexpr (std::is_same_v<T, std::vector<std::any>>) {
		output += GetFormattedString(obj, depth);
	} else if constexpr (std::is_same_v<T, std::map<std::string, std::any>>) {
		output += GetFormattedString(obj, depth);
	} else {
		throw std::runtime_error("unable to encode type");
	}

	return output;
}

//private
std::string GetFormattedString(const std::vector<unsigned char>& obj) {
	std::ostringstream result;
	result << std::hex << std::setfill('0');
	
	for(const auto& element : obj) {
		result << std::setw(2) << static_cast<int>(element);
	}

	result << " (" << std::string(obj.begin(), obj.end()) << ")";

	return result.str();
}

//private
std::string GetFormattedString(long obj) {
	std::ostringstream ss;
	ss << obj;
	return ss.str();
}

//private
template<typename T>
std::string GetFormattedString(const std::vector<T>& obj, int depth) {
	std::string pad1(depth * 2, ' ');
	std::string pad2((depth + 1) * 2, ' ');

	if(obj.empty())
		return "[]";

	if (std::is_same<T, std::map<std::string, T>>::value)
		return "\n" + pad1 + "[" + std::accumulate(obj.begin(), obj.end(), std::string(""), [depth, pad2](const std::string& a, const T& b) {
			return a + pad2 + GetFormattedString(b, depth + 1);
		}) + "\n" + pad1 + "]";

	return "[ " + std::accumulate(obj.begin(), obj.end(), std::string(""), [](const std::string& a, const T& b) {
		return a + GetFormattedString(b);
	}) + " ]";
}