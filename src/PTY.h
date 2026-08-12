class PTY {
public:
  virtual void start_shell(char *shell) = 0;

  virtual ~PTY(void) = default;

  static PTY &getPTY(void);
  // bool 変換
  virtual explicit operator bool() const noexcept = 0;
};
