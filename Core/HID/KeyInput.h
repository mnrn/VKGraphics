/**
 * @brief  キー入力
 * @date   2020/12/14
 */

#ifndef KEY_INPUT_H
#define KEY_INPUT_H

// ********************************************************************************
// Including files
// ********************************************************************************

#include <GLFW/glfw3.h>

#include <cstdint>
#include <vector>

#include "Utils/Singleton.h"

// ********************************************************************************
// Classes
// ********************************************************************************

/**< @brief 入力タイプのビットフラグ用列挙型構造体 */
enum struct InputType : std::uint32_t {
  Keyboard = 0x01, /**< キーボード入力 */
  // Joypad = 0x02,   /**< パッド入力 */
};

/**< @brief キービットフラグ用列挙型構造体 */
enum struct Key : std::uint32_t {
  None = 0x0000,

  Down = 0x0001,  /**< 下キーチェックマスク         */
  Left = 0x0002,  /**< 左キーチェックマスク         */
  Right = 0x0004, /**< 右キーチェックマスク         */
  Up = 0x0008,    /**< 上キーチェックマスク         */

  Ret = (0x0001 << 8), /**< リターンキーチェックマスク   */
  Sft = (0x0002 << 8), /**< シフトキーチェックマスク     */
  Esc = (0x0004 << 8), /**< エスケープキーチェックマスク */
  Spc = (0x0008 << 8), /**< スペースキーチェックマスク   */

  Z = (0x0001 << 12), /**< Zキーチェックマスク          */
  X = (0x0002 << 12), /**< Xキーチェックマスク          */
  C = (0x0004 << 12), /**< Cキーチェックマスク          */
  A = (0x0008 << 12), /**< Aキーチェックマスク          */

  S = (0x0001 << 16), /**< Sキーチェックマスク          */
  D = (0x0002 << 16), /**< Dキーチェックマスク          */
  Q = (0x0004 << 16), /**< Qキーチェックマスク          */
  W = (0x0008 << 16), /**< Wキーチェックマスク          */
};

/**< @brief キー入力に関するデータテーブル作成用構造体 */
struct KeyData {
  Key flg;                   /**< キー入力チェックフラグ */
  std::uint32_t virtualKey;  /**< 仮想キーコードその1    */
  std::uint32_t virtualKey2; /**< 仮想キーコードその2    */
};

class KeyInput : public Singleton<KeyInput> {
public:
  KeyInput() {
    state_.fresh = 0;
    state_.old = 0;
    state_.trg = 0;
    state_.type = static_cast<std::uint32_t>(InputType::Keyboard);
  }
  explicit KeyInput(std::uint32_t type) {
    state_.fresh = 0;
    state_.old = 0;
    state_.trg = 0;
    state_.type = type;
  }

  void OnUpdate(GLFWwindow *hwd) {
    // キー入力初期化
    state_.old = state_.fresh;
    state_.fresh = 0x00;

    // キー入力更新
    if (state_.type & static_cast<std::uint32_t>(InputType::Keyboard)) {
      UpdateKeyboard(hwd);
    }

    // トリガーを更新
    state_.trg = state_.fresh ^ (state_.old & state_.fresh);
  }

  void SetInputType(InputType type) {
    state_.type |= static_cast<std::uint32_t>(type);
  }
  bool IsFresh(Key key) const {
    return (state_.fresh & static_cast<std::uint32_t>(key)) != 0;
  }
  bool IsTrg(Key key) const {
    return (state_.trg & static_cast<std::uint32_t>(key)) != 0;
  }

private:
  /**< @brief キー入力情報格納用構造体 */
  struct KeyState {
    std::uint32_t fresh; /**< キー入力格納変数       */
    std::uint32_t old;   /**< キー入力格納変数       */
    std::uint32_t trg;   /**< トリガー格納変数       */
    std::uint32_t type;  /**< キー入力のタイプ       */
  };

  void UpdateKeyboard(GLFWwindow *hwd) {
    for (const auto &keyData : kKeyDataTbl) {
      if (glfwGetKey(hwd, keyData.virtualKey) == GLFW_PRESS ||
          (keyData.virtualKey2 != 0x00 &&
           glfwGetKey(hwd, keyData.virtualKey2) == GLFW_PRESS)) {
        state_.fresh |= static_cast<std::uint32_t>(keyData.flg);
      }
    }
  }

  KeyState state_;

  static const inline std::vector<KeyData> kKeyDataTbl = {
      {Key::Down, GLFW_KEY_DOWN, 0x00},
      {Key::Spc, GLFW_KEY_SPACE, 0x00},
      {Key::Left, GLFW_KEY_LEFT, 0x00},
      {Key::Esc, GLFW_KEY_ESCAPE, 0x00},
      {Key::Right, GLFW_KEY_RIGHT, 0x00},
      {Key::Sft, GLFW_KEY_LEFT_SHIFT, GLFW_KEY_RIGHT_SHIFT},
      {Key::Up, GLFW_KEY_UP, 0x00},
      {Key::Ret, GLFW_KEY_ENTER, 0x00},
      {Key::Z, GLFW_KEY_Z, 0x00},
      {Key::S, GLFW_KEY_S, 0x00},
      {Key::X, GLFW_KEY_X, 0x00},
      {Key::D, GLFW_KEY_D, 0x00},
      {Key::C, GLFW_KEY_C, 0x00},
      {Key::Q, GLFW_KEY_Q, 0x00},
      {Key::A, GLFW_KEY_A, 0x00},
      {Key::W, GLFW_KEY_W, 0x00},
  };
};

#endif
