/* Copyright (c) 2015 Digiverse d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License. The
 * license should be included in the source distribution of the Software;
 * if not, you may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * The above copyright notice and licensing terms shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#if !defined(DLL_DECL_H_HEADER_GUARD)
#define DLL_DECL_H_HEADER_GUARD

#if !defined(WIN32_TARGET)
#  if defined(_MSC_VER)
#    define WIN32_TARGET
#  endif
#endif

#if !defined(APPLE_TARGET)
#  if defined(__APPLE__)
#    define APPLE_TARGET
#  endif
#endif

#if !defined(LINUX_TARGET)
#  if defined(__linux)
#    define LINUX_TARGET
#  endif
#endif

#if defined(WIN32_TARGET)
#  if defined(WIN32_COOL_BUILD)
#    define dlldecl __declspec( dllexport )
#  else
#    define dlldecl __declspec( dllimport )
#  endif
#else
#  define dlldecl
#endif

#if defined(WIN32_TARGET)
#  define INCORRECT_VARIADIC
#endif

#endif
