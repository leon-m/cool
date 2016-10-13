/* Copyright (c) 2016 Digiverse d.o.o.
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

#if !defined(cool_a8ed9ec1_c086_4866_8e9c_4de137df0daf)
#define cool_a8ed9ec1_c086_4866_8e9c_4de137df0daf

#if !defined(COOL_ASYNC_PLATFORM)
# error "COOL_ASYNC_PLATFORM must be defined and set to one of supported platforms"
#endif


#if COOL_ASYNC_PLATFORM == gcd
#  include "gcd/runner.h"
#elif COOL_USE_ASYNC_PLATFORM == winwtp
#  include "winwtp/runner.h"
#else
# error "unsupported async platform" COOL_USE_ASYNC_PLATFORM
#endif


#endif
