/**
 * @note スレッドセーフではありません。
 * 各自、mutex など用いてください。
 */

#pragma once

#include <memory>

/**
 * @brief シングルトンの基底クラス
 */
template <class T> class Singleton {
public:
  /**
   * @brief  インスタンスの生成
   * @tparam ... Args        可変長テンプレートパラメータ
   * @param  Args&&... args  コンストラクタの引数
   */
  template <class... Args> static T &Get(Args &&... args) {
    if (instance_ == nullptr) {
      Create(std::forward<Args>(args)...);
    }
    return *instance_.get();
  }

  /**
   * @brief  インスタンスの生成
   * @tparam ... Args        可変長テンプレートパラメータ
   * @param  Args&&... args  コンストラクタの引数
   */
  template <class... Args> static void Create(Args &&... args) {
    if (instance_ != nullptr) {
      return;
    }
    instance_ = std::make_unique<T>(std::forward<Args>(args)...);
  }

  /**< @brief インスタンスの破棄 */
  static void Destroy() {
    if (instance_ != nullptr) {
      decltype(auto) res = instance_.release();
      delete res;
      instance_ = nullptr;
    }
  }

  /**< @brief インスタンスが存在するか判定 */
  static bool IsExist() { return instance_ != nullptr; }

protected:
  // --------------------------------------------------------------------------------
  // 特殊メンバ関数
  // --------------------------------------------------------------------------------

  explicit Singleton<T>() = default;  /**< @brief コンストラクタ */
  ~Singleton<T>() noexcept = default; /**< @brief デストラクタ   */

private:
  Singleton<T>(const Singleton<T> &) = delete;
  Singleton<T> &operator=(const Singleton<T> &) = delete;

  // --------------------------------------------------------------------------------
  // 静的メンバ変数
  // --------------------------------------------------------------------------------

  static inline std::unique_ptr<T> instance_ =
      nullptr; /**< インスタンス本体   */
};
