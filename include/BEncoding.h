#ifndef BENCODING_H
#define BENCODING_H

namespace BitTorrent {
	class BEncoding {
		public:
			static constexpr char DictStart = 'd';
			static constexpr char DictEnd = 'e';
			static constexpr char ListStart = 'l';
			static constexpr char ListEnd = 'e';
			static constexpr char NumStart = 'i';
			static constexpr char NumEnd = 'e';
			static constexpr char ByteArrDiv = ':';

			static std::any Decode(const std::vector<char>& bytes);

		private:
			static std::any DecodeNextObj(std::vector<char>::const_iterator& it);
	};
	
	class MemoryStreamExt {
		public:
			static void Append(std::vector<char>& stream, char val);
			static void Append(std::vector<char>& stream, const std::vector<char>& vals);
	}

}

#endif // BENCODING_H
