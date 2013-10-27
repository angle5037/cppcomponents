//          Copyright John R. Bandela 2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstddef>
#include <string>

#ifdef CPPCOMPONENTS_WINRT_APP
#include <cwchar>
#include <vector>
#endif


// On Windows use stdcall
#define CROSS_CALL_CALLING_CONVENTION __stdcall
#define CROSS_CALL_EXPORT_FUNCTION __declspec(dllexport)
namespace cross_compiler_interface{
 
    namespace detail{
        // Define Win32 functions so we will not have to include windows.h for 3 functions
        namespace Windows{
            struct WindowsMODULE{
                int unused;
            };
            typedef WindowsMODULE* WindowsHModule;
            extern "C"{
#ifndef CPPCOMPONENTS_WINRT_APP
                __declspec(dllimport) 
                    WindowsHModule
                    __stdcall
                    LoadLibraryA(
                    const char* lpLibFileName
                    );
#else
				WindowsHModule __stdcall LoadPackagedLibrary(
					        const wchar_t* lpwLibFileName,
					  std::uint32_t Reserved
					);
#endif
 
                typedef int (__stdcall  *Proc)();
                __declspec(dllimport)
                    Proc
                    __stdcall
                    GetProcAddress(
                    WindowsHModule hModule,
                    const char* lpProcName
                    );
 
                __declspec(dllimport)
                    int
                    __stdcall
                    FreeLibrary(
                    WindowsHModule hLibModule
                    );
 
            }
        }
    }
    class module{
 
        detail::Windows::WindowsHModule m_;
 
        // Not copyable
        module(const module&);
        module& operator=(const module&);




		template<class F,std::size_t sz>
		F load_module_function_options(std::string (&funcs)[sz] )const{
			for (const auto& func : funcs){
				auto f = reinterpret_cast<F>(detail::Windows::GetProcAddress(m_, func.c_str()));
				if (f)return f;
			}
			throw error_shared_function_not_found();
		}   
	public:
		// Movable
		module(module && other) : m_(other.m_){
			other.release();
		}

		module& operator=(module && other){
			m_ = other.m_;
			other.release();
			return *this;
		}	
		
		module( std::string m) : m_(nullptr){
			// if empty string, leave m_ as nullptr;
			if (m.empty()) return;
            // if there is a \ or / in the string then pass it unmodified
            // otherwise place a .dll extension
            if(m.find('\\') == std::string::npos && m.find('/')==std::string::npos){
                // add a .dll extension
                m += ".dll";
            }
#ifndef CPPCOMPONENTS_WINRT_APP
            m_ = detail::Windows::LoadLibraryA(m.c_str());
#else
			auto mbstr = m.c_str();
			std::mbstate_t state = std::mbstate_t();
			auto len = 1 + std::mbsrtowcs(NULL, &mbstr, 0, &state);
			std::vector<wchar_t> wstr(len);
			std::mbsrtowcs(&wstr[0], &mbstr, wstr.size(), &state);
			wchar_t* ws = &wstr[0];
			m_ = detail::Windows::LoadPackagedLibrary(ws,0);
#endif
            if(!m_){
                throw error_unable_to_load_library();
            }
 
 
        };
 

		template<class F>
		F load_module_function(const std::string& func)const{
			auto f = reinterpret_cast<F>(detail::Windows::GetProcAddress(m_, func.c_str()));
			if (!f){
				std::string funcs [] = {
					"_" + func,
					func + "@0",
					func + "@4",
					func + "@8",
					func + "@16",
					"_" + func + "@0",
					"_" + func + "@4",
					"_" + func + "@8",
					"_" + func + "@16"
				};
				return load_module_function_options<F>(funcs);
			}

			return f;
		}
		bool valid(){
			return m_ != nullptr;
		}
        void release(){
            m_ = nullptr;
        }
        ~module(){
            if(m_)detail::Windows::FreeLibrary(m_);
        };
    };
 
}