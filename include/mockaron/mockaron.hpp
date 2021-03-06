#ifndef MOCKARON_HPP
#define MOCKARON_HPP

#include <mockaron/detail/typetraits.hpp>

#include <iostream>
#include <unordered_map>
#include <string>
#include <typeinfo>
#include <functional>

#if defined(_WIN32) && defined(mockaron_EXPORTS)
#define MOCKARON_EXPORT __declspec(dllexport)
#else
#define MOCKARON_EXPORT
#endif

/// Private macro
#define MOCKARON_NOTHING
/// Private macro
#define MOCKARON_ADD_COMMA(...) , __VA_ARGS__
/// Private macro
#define MOCKARON_CONCAT_I(a, b) a ## b
/// Private macro
#define MOCKARON_CONCAT(a, b) MOCKARON_CONCAT_I(a, b)

/** Set a function implementation for a method mock
 *
 * \param cl the class to mock
 * \param func method to mock
 * \param hndl functor to run when the method is called
 */
#define MOCKARON_SET_IMPL(cl, func, hndl)                           \
  mock_impl::add_hook<                                              \
      ::mockaron::detail::remove_class_ptr_t<decltype(&cl::func)>>( \
      #func, hndl)

/** Set a function implementation for a mock in presence of overload
 *
 * Same as MOCKARON_SET_IMPL with support for overload.
 *
 * \param sig the signature of the method to mock
 *
 * This macro will trigger a compilation error if there is no such signature for
 * the given method.
 */
#define MOCKARON_SET_IMPL_SIG(sig, cl, func, hndl)                   \
  do                                                                 \
  {                                                                  \
    /* check that the function exists */                             \
    (void)static_cast<::mockaron::detail::add_class_ptr_t<cl, sig>>( \
        &cl::func);                                                  \
    mock_impl::add_hook<sig>(#func, hndl);                           \
  } while (0)

/** Same as MOCKARON_SET_IMPL_SIG but with a custom return type.
 *
 * Useful for coroutine code.
 */
#define MOCKARON_SET_IMPL_CUSTOM(sig, ret, cl, func, hndl)           \
  do                                                                 \
  {                                                                  \
    /* check that the function exists */                             \
    (void)static_cast<::mockaron::detail::add_class_ptr_t<cl, sig>>( \
        &cl::func);                                                  \
    mock_impl::add_hook<sig, ret>(#func, hndl);                      \
  } while (0)

/** Declare an implementation of a method
 *
 * \param cl the class to mock
 * \param func the method to mock
 */
#define MOCKARON_DECLARE_IMPL(cl, func)                                  \
  MOCKARON_SET_IMPL(cl, func, [this](auto&&... args) -> decltype(auto) { \
    return this->func(std::forward<decltype(args)>(args)...);            \
  });

/** Declare an implementation of a method in presence of overload
 *
 * Same as MOCKARON_DECLARE_IMPL with support for overload.
 *
 * \param sig the signature of the method to mock
 */
#define MOCKARON_DECLARE_IMPL_SIG(sig, cl, func)                  \
  MOCKARON_SET_IMPL_SIG(                                          \
      sig, cl, func, [this](auto&&... args) -> decltype(auto) {   \
        return this->func(std::forward<decltype(args)>(args)...); \
      });

/** Same as MOCKARON_DECLARE_IMPL_SIG but with a custom return type.
 *
 * Useful for coroutine code.
 */
#define MOCKARON_DECLARE_IMPL_CUSTOM(sig, ret, cl, func)             \
  MOCKARON_SET_IMPL_CUSTOM(                                          \
      sig, ret, cl, func, [this](auto&&... args) -> decltype(auto) { \
        return this->func(std::forward<decltype(args)>(args)...);    \
      });

/** Set a function implementation for a function mock
 *
 * \param func function to mock
 * \param hndl functor to run when the function is called
 */
#define MOCKARON_SET_FUNCTION_IMPL(func, hndl)                         \
  ::mockaron::detail::raii_mock<std::remove_pointer_t<decltype(func)>> \
      MOCKARON_CONCAT(_mockaron_function_mock, __LINE__)(              \
          reinterpret_cast<void const*>(func), hndl)

/* Same as MOCKARON_SET_FUNCTION_IMPL but with overload support and a custom
 * return type.
 */
#define MOCKARON_SET_FUNCTION_IMPL_CUSTOM(sig, ret, func, hndl) \
  ::mockaron::detail::raii_mock<                                \
      ::mockaron::detail::replace_return_type_t<sig, ret>>      \
      MOCKARON_CONCAT(_mockaron_function_mock, __LINE__)(       \
          reinterpret_cast<void const*>(                        \
              static_cast<std::add_pointer_t<sig>>(func)),      \
          hndl)

#if MOCKARON_DISABLE_HOOKS
#define MOCKARON_HOOK(...)
#define MOCKARON_HOOK0(...)
#define MOCKARON_HOOK_CUSTOM(...)
#define MOCKARON_HOOK_SIG(...)
#define MOCKARON_HOOK_SIG0(...)
#define MOCKARON_FUNCTION_HOOK_CUSTOM(...)
#define MOCKARON_FUNCTION_HOOK(...)
#define MOCKARON_FUNCTION_HOOK0(...)
#else

/** Hook a method with mockaron
 *
 * \param cl the class
 * \param func the method
 * \param ... the arguments received
 */
#define MOCKARON_HOOK(cl, func, ...)                               \
  MOCKARON_HOOK_SIG(                                               \
      ::mockaron::detail::remove_class_ptr_t<decltype(&cl::func)>, \
      cl,                                                          \
      func,                                                        \
      __VA_ARGS__)

/** Hook a method without arguments with mockaron
 *
 * Same as MOCKARON_HOOK for methods without arguments.
 */
#define MOCKARON_HOOK0(cl, func) \
  MOCKARON_HOOK_SIG0(            \
      ::mockaron::detail::remove_class_ptr_t<decltype(&cl::func)>, cl, func)

/** Same as MOCKARON_HOOK_SIG but with a custom return type.
 *
 * Useful for coroutine code.
 *
 * Usage:
 *
 *   MOCKARON_HOOK_CUSTOM(// function signature
 *                        tc::cotask<PullResult>(gsl::span<GroupId const>),
 *                        // real return type
 *                        PullResult,
 *                        // class name
 *                        GroupAccessor,
 *                        // function name
 *                        pull,
 *                        // return, co_return or a function-macro
 *                        TC_RETURN,
 *                        // arguments
 *                        MOCKARON_ADD_COMMA(groupIds));
 */
#define MOCKARON_HOOK_CUSTOM(sig, ret, cl, func, _return, exp)          \
  do                                                                    \
  {                                                                     \
    (void)static_cast<::mockaron::detail::add_class_ptr_t<cl, sig>>(    \
        &cl::func);                                                     \
    if (!::mockaron::detail::is_a_mock(this))                           \
      break;                                                            \
    _return((::mockaron::detail::run_hook<sig, ret>(                    \
        *reinterpret_cast<::mockaron::detail::mock_impl* const*>(this), \
        #func exp)));                                                   \
  } while (0)

/** Same as MOCKARON_HOOK_FUNCTION but with support for overload and a custom
 * return type.
 *
 * Usage:
 *
 *   MOCKARON_FUNCTION_HOOK_CUSTOM(
 *       // function signature
 *       tc::cotask<PullResult>(gsl::span<GroupId const>),
 *       // real return type
 *       PullResult,
 *       // function name
 *       pull,
 *       // return, co_return or a function-macro
 *       TC_RETURN,
 *       // arguments
 *       groupIds);
 */
#define MOCKARON_FUNCTION_HOOK_CUSTOM(sig, ret, func, _return, ...)           \
  do                                                                          \
  {                                                                           \
    auto const mock = ::mockaron::detail::get_function_hook(                  \
        reinterpret_cast<void*>(static_cast<std::add_pointer_t<sig>>(func))); \
    if (!mock)                                                                \
      break;                                                                  \
    _return((mock->get<::std::function<                                       \
                 ::mockaron::detail::replace_return_type_t<sig, ret>>>()(     \
        __VA_ARGS__)));                                                       \
  } while (0)

/** Hook a function with mockaron
 *
 * \param func the function
 * \param ... the arguments received
 */
#define MOCKARON_FUNCTION_HOOK(func, ...)                                     \
  do                                                                          \
  {                                                                           \
    auto const mock =                                                         \
        ::mockaron::detail::get_function_hook(reinterpret_cast<void*>(func)); \
    if (!mock)                                                                \
      break;                                                                  \
    return mock                                                               \
        ->get<::std::function<std::remove_pointer_t<decltype(func)>>>()(      \
            __VA_ARGS__);                                                     \
  } while (0)

/** Hook a function without arguments with mockaron
 *
 * Same as MOCKARON_FUNCTION_HOOK for functions without arguments.
 */
#define MOCKARON_FUNCTION_HOOK0(func) \
  MOCKARON_FUNCTION_HOOK(func, MOCKARON_NOTHING)

/** Hook a method in prensence of overload
 *
 * Same as MOCKARON_HOOK with support for overload.
 *
 * \param sig the signature of the method to mock
 */
#define MOCKARON_HOOK_SIG(sig, cl, func, ...)             \
  MOCKARON_HOOK_CUSTOM(                                   \
      sig,                                                \
      ::mockaron::detail::return_type_t<                  \
          ::mockaron::detail::remove_abomination_t<sig>>, \
      cl,                                                 \
      func,                                               \
      return,                                             \
      MOCKARON_ADD_COMMA(__VA_ARGS__))

/** Hook a method without argument in presence of overload
 *
 * Same as MOCKARON_HOOK_SIG for methods without arguments.
 */
#define MOCKARON_HOOK_SIG0(sig, cl, func)                 \
  MOCKARON_HOOK_CUSTOM(                                   \
      sig,                                                \
      ::mockaron::detail::return_type_t<                  \
          ::mockaron::detail::remove_abomination_t<sig>>, \
      cl,                                                 \
      func,                                               \
      return,                                             \
      MOCKARON_NOTHING)
#endif

namespace mockaron
{
namespace detail
{

class any
{
public:
  any() = default;
  template <typename T>
  any(T&& v)
  {
    set(std::forward<T>(v));
  }
  any(any&& o)
    : _ptr(o._ptr),
      _deleter(std::move(o._deleter))
  {
    o._ptr = nullptr;
  }
  any& operator=(any&& o)
  {
    _ptr = o._ptr;
    _deleter = std::move(o._deleter);
    o._ptr = nullptr;
    return *this;
  }
  ~any()
  {
    if (_ptr)
      _deleter(_ptr);
  }

  template <typename T>
  void set(T&& v)
  {
    _ptr = new std::decay_t<T>(std::forward<T>(v));
    _deleter = [](void* ptr) { delete static_cast<T*>(ptr); };
  }

  template <typename T>
  T& get()
  {
    return *static_cast<T*>(_ptr);
  }

private:
  void* _ptr{nullptr};
  std::function<void(void*)> _deleter;
};

template <typename F>
struct return_type;

template <typename R, typename... Args>
struct return_type<R(Args...)>
{
  using type = R;
};

template <typename F>
using return_type_t = typename return_type<F>::type;

template <typename F, typename R2>
struct replace_return_type;

template <typename R, typename... Args, typename R2>
struct replace_return_type<R(Args...), R2>
{
  using type = R2(Args...);
};

template <typename F, typename R2>
using replace_return_type_t = typename replace_return_type<F, R2>::type;

class mock_impl
{
public:
  virtual ~mock_impl() = default;

protected:
  template <
      typename Sig,
      typename R = return_type_t<remove_abomination_t<Sig>>,
      typename H>
  void add_hook(char const* name, H&& handler)
  {
    add_hook_(
        name,
        typeid(wrap<Sig>).name(),
        any(std::function<replace_return_type_t<remove_abomination_t<Sig>, R>>(
            std::forward<H>(handler))));
  }

private:
  using map_type = std::unordered_map<std::string, any>;
  map_type _methods;

  MOCKARON_EXPORT void add_hook_(char const* name, char const* sig, any h);

  template <typename Sig, typename Ret, typename... Args>
  friend decltype(auto) run_hook(
      mock_impl* mi, char const* name, Args&&... args);
};

template <
    typename Sig,
    typename Ret,
    typename... Args>
decltype(auto) run_hook(mock_impl* mi, char const* name, Args&&... args)
{
  auto const n =
      std::string(name) + "##" + typeid(wrap<Sig>).name();
  if (!mi->_methods.count(n))
  {
    std::cerr << n << " is not mocked! See README for more information." <<
      std::endl;
    std::terminate();
  }
  return mi->_methods[n]
      .get<std::function<
          replace_return_type_t<remove_abomination_t<Sig>, Ret>>>()(
          std::forward<Args>(args)...);
}

MOCKARON_EXPORT void register_mock(void const* p);
MOCKARON_EXPORT void unregister_mock(void const* p);
MOCKARON_EXPORT bool is_a_mock(void const* v);

MOCKARON_EXPORT any* get_function_hook(void const* n);
MOCKARON_EXPORT void register_function_mock(void const* n, any h);
MOCKARON_EXPORT void unregister_function_mock(void const* n);

template <typename Sig>
class raii_mock
{
public:
  raii_mock(void const* reff, std::function<Sig> f) : ref(reff)
  {
    register_function_mock(ref, std::move(f));
  }
  ~raii_mock()
  {
    unregister_function_mock(ref);
  }

private:
  void const* ref;
};

}

/// Class to inherit from to make a mock
using mock_impl = detail::mock_impl;

/** Class to instantiate to create a mock
 *
 * \tparam T the class to mock
 * \tparam M the mock implementation
 */
template <typename T, typename M>
class mock
{
public:
  mock(mock const&) = delete;
  mock& operator=(mock const&) = delete;
  mock(mock&&) = delete;
  mock& operator=(mock&&) = delete;

  mock()
  {
    detail::register_mock(&_p);
    _p = new M();
  }
  ~mock()
  {
    delete _p;
    detail::unregister_mock(&_p);
  }
  T& get()
  {
    return *get_fake();
  }
  M& get_mock_impl()
  {
    return *static_cast<M*>(_p);
  }

private:
  detail::mock_impl* _p;

  T* get_fake()
  {
    return reinterpret_cast<T*>(&_p);
  }
};

}

#endif
