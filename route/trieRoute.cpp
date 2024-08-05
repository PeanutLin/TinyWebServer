#include "trieRoute.h"
#include <sstream>
#include "../utils/utils.h"

// 将 string 按照 delimiter 划分
std::vector<std::string> split(const std::string& str, char delimiter) {
  std::vector<std::string> tokens;
  std::string token;
  std::stringstream ss(str.c_str());

  while (std::getline(ss, token, delimiter)) {
    if (token != "") {
      tokens.push_back(token);
    }
  }

  return tokens;
}

// 构造函数
Trie::Trie() {
  root = new Node;
  root->isEnd = true;
  root->exactMatch = true;
}
// 析构函数
Trie::~Trie() {
  delete root;
}

void Trie::insert(std::string path) {
  std::vector<std::string> tokens = split(path, '/');
  Node* now = root;
  for (auto token : tokens) {
    auto temps = now->child;
    Node* nxt = nullptr;
    for (auto temp : temps) {
      if (temp->part == token) {
        nxt = temp;
        break;
      }
    }
    if (nxt == nullptr) {
      nxt = new Node;
      nxt->part = token;
      nxt->exactMatch = true;
      nxt->isEnd = false;

      if (token[0] == ':') {
        nxt->exactMatch = false;
      }
      now->child.push_back(nxt);
    }
    now = nxt;
  }
  now->isEnd = true;
}

bool Trie::search(std::string path, std::map<std::string, std::string>& mp) {
  std::vector<std::string> tokens = split(path, '/');
  Node* now = root;
  for (auto token : tokens) {
    auto temps = now->child;
    bool isOk = false;
    for (auto temp : temps) {
      if (!temp->exactMatch) {
        std::string var = temp->part.substr(1);
        mp[var] = token;
        now = temp;
        isOk = true;
        break;
      } else if (temp->part == token) {
        now = temp;
        isOk = true;
        break;
      } else {
        isOk = false;
        break;
      }
    }
    if (!isOk) {
      return false;
    }
  }
  return true;
}