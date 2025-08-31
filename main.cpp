#include <iostream>
#include <fstream>
#include <map>
#include <queue>
#include <string>
#include <vector>
#include <cstdint>

using namespace std;

class BitStream {
private:
    ostream* out_;
    istream* in_;
    bool is_writer_;
    char buffer_;
    int bit_pos_;
    bool eof_flag_;

public:
    BitStream(ostream& out)
        : out_(&out), in_(nullptr), is_writer_(true), buffer_(0), bit_pos_(0), eof_flag_(false) {}

    BitStream(istream& in)
        : out_(nullptr), in_(&in), is_writer_(false), buffer_(0), bit_pos_(8), eof_flag_(false) {}

   
    void writeBit(bool bit) {
        if (!is_writer_) return;
        buffer_ <<= 1;
        if (bit) buffer_ |= 1;
        bit_pos_++;
        if (bit_pos_ == 8) {
            out_->put(buffer_);
            buffer_ = 0;
            bit_pos_ = 0;
        }
    }

 
    void writeByte(uint8_t byte) {
        if (is_writer_) {
            if (bit_pos_ == 0) {
                out_->put(byte);
            } else {
               
                for (int i = 7; i >= 0; --i) {
                    writeBit((byte >> i) & 1);
                }
            }
        }
    }

    bool readBit() {
        if (is_writer_ || eof_flag_) return false;
        if (bit_pos_ == 8) {
            if (!in_->get(buffer_)) {
                eof_flag_ = true;
                return false;
            }
            bit_pos_ = 0;
        }
        bool bit = (buffer_ & (1 << (7 - bit_pos_))) != 0;
        bit_pos_++;
        return bit;
    }
    
    
    uint8_t readByte() {
        uint8_t byte = 0;
        for (int i = 0; i < 8; ++i) {
            if (eof_flag_) return 0;
            byte <<= 1;
            if (readBit()) {
                byte |= 1;
            }
        }
        return byte;
    }

   
    void flush() {
        if (is_writer_ && bit_pos_ > 0) {
            buffer_ <<= (8 - bit_pos_);
            out_->put(buffer_);
            buffer_ = 0;
            bit_pos_ = 0;
        }
    }

    bool eof() const { return eof_flag_; }
};


struct Node {
    char data;
    int freq;
    Node* left;
    Node* right;

    Node(char d = '\0', int f = 0) : data(d), freq(f), left(nullptr), right(nullptr) {}
};

void deleteTree(Node* node) {
    if (node) {
        deleteTree(node->left);
        deleteTree(node->right);
        delete node;
    }
}

struct CompareNode {
    bool operator()(Node* a, Node* b) {
        return a->freq > b->freq;
    }
};

map<char, string> huffman_codes;


Node* buildHuffmanTree(map<char, int>& freq_map) {
    priority_queue<Node*, vector<Node*>, CompareNode> pq;

    for (auto const& [ch, freq] : freq_map) {
        pq.push(new Node(ch, freq));
    }

    while (pq.size() > 1) {
        Node* left = pq.top(); pq.pop();
        Node* right = pq.top(); pq.pop();

        Node* internal = new Node('$', left->freq + right->freq);
        internal->left = left;
        internal->right = right;

        pq.push(internal);
    }

    if (pq.empty()) return nullptr;
    return pq.top();
}


void generateCodes(Node* root, string code) {
    if (!root) return;

    if (!root->left && !root->right) {
        huffman_codes[root->data] = code;
        return;
    }

    generateCodes(root->left, code + "0");
    generateCodes(root->right, code + "1");
}


void writeCodebook(BitStream& bit_writer, const map<char, string>& codes) {
    uint16_t size = codes.size();
    bit_writer.writeByte((size >> 8) & 0xFF);
    bit_writer.writeByte(size & 0xFF);

    for (const auto& pair : codes) {
        char ch = pair.first;
        const string& code = pair.second;

        bit_writer.writeByte(ch);
        uint8_t code_len = code.size();
        bit_writer.writeByte(code_len);

        for (char bit : code) {
            bit_writer.writeBit(bit == '1');
        }
    }
}


Node* readCodebookAndBuildTree(BitStream& bit_reader) {
    Node* root = new Node();
    uint16_t size;
    size = bit_reader.readByte() << 8;
    size |= bit_reader.readByte();

    for (uint16_t i = 0; i < size; ++i) {
        char ch = bit_reader.readByte();
        uint8_t code_len = bit_reader.readByte();

        string code = "";
        for (int j = 0; j < code_len; ++j) {
            code += bit_reader.readBit() ? '1' : '0';
        }

        Node* curr = root;
        for (char bit : code) {
            if (bit == '0') {
                if (!curr->left) curr->left = new Node();
                curr = curr->left;
            } else {
                if (!curr->right) curr->right = new Node();
                curr = curr->right;
            }
        }
        curr->data = ch;
    }
    return root;
}


void writeCompressedData(ifstream& input_file, BitStream& bit_writer, const map<char, string>& codes) {
    char ch;
    input_file.clear();
    input_file.seekg(0, ios::beg);

    while (input_file.get(ch)) {
        const string& code = codes.at(ch);
        for (char bit : code) {
            bit_writer.writeBit(bit == '1');
        }
    }
    bit_writer.flush();
}


void decodeData(Node* root, BitStream& bit_reader, ofstream& decompressed_output, long long original_size) {
    Node* curr = root;

    long long decoded_count = 0;
    while (decoded_count < original_size) {
        bool bit = bit_reader.readBit();
        if (bit_reader.eof()) break; 
        
        curr = bit ? curr->right : curr->left;

        if (!curr->left && !curr->right) {
            decompressed_output.put(curr->data);
            curr = root;
            decoded_count++;
        }
    }
}


int main() {
    
    cout << "Starting compression..." << endl;
    ifstream input_file("input.txt", ios::binary);
    if (!input_file.is_open()) {
        cerr << "Failed to open input file 'input.txt'!" << endl;
        return 1;
    }

    map<char, int> freq_map;
    long long char_count = 0;
    char ch;
    while (input_file.get(ch)) {
        freq_map[ch]++;
        char_count++;
    }

    if (freq_map.empty()) {
        cerr << "Input file is empty. Nothing to compress." << endl;
        input_file.close();
        return 1;
    }

    Node* root = buildHuffmanTree(freq_map);
    if (root == nullptr) {
        cerr << "Failed to build Huffman tree." << endl;
        input_file.close();
        return 1;
    }
    generateCodes(root, "");


    cout << "Huffman Codes:" << endl;
    for (auto& [ch, code] : huffman_codes) {
        if (ch >= 32 && ch <= 126)
            cout << "'" << ch << "': " << code << endl;
        else
            cout << "ASCII(" << int(static_cast<unsigned char>(ch)) << "): " << code << endl;
    }

    ofstream compressed_file("output.bin", ios::binary);
    if (!compressed_file.is_open()) {
        cerr << "Failed to open output file 'output.bin'!" << endl;
        deleteTree(root);
        return 1;
    }

   
    compressed_file.write(reinterpret_cast<const char*>(&char_count), sizeof(long long));

  
    BitStream bit_writer(compressed_file);

   
    writeCodebook(bit_writer, huffman_codes);
    bit_writer.flush(); 

    
    writeCompressedData(input_file, bit_writer, huffman_codes);

    input_file.close();
    compressed_file.close();
    cout << "Compression complete! Output written to output.bin" << endl;
    deleteTree(root);

    
    cout << "\nStarting decompression..." << endl;
    ifstream compressed_input("output.bin", ios::binary);
    ofstream decompressed_output("decompressed.txt", ios::binary);

    if (!compressed_input.is_open() || !decompressed_output.is_open()) {
        cerr << "Failed to open compressed or decompressed file!" << endl;
        return 1;
    }

    long long original_size;
    compressed_input.read(reinterpret_cast<char*>(&original_size), sizeof(long long));

    
    BitStream bit_reader(compressed_input);

   
    Node* decode_root = readCodebookAndBuildTree(bit_reader);
    
   
    decodeData(decode_root, bit_reader, decompressed_output, original_size);

    compressed_input.close();
    decompressed_output.close();
    deleteTree(decode_root);

    cout << "Decompression complete! Output written to decompressed.txt" << endl;
    return 0;
}
