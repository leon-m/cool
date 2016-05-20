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

#if !defined(ENTRAILS_IP_ADDRESS_H_HEADER_GUARD)
#define ENTRAILS_IP_ADDRESS_H_HEADER_GUARD

#include <initializer_list>
#include "cool/exception.h"
#include "cool/binary.h"
#include <cstring>

namespace cool { namespace net {

enum class style;

namespace entrails {


template <std::size_t Size>
cool::basis::binary<Size> calculate_mask(std::size_t length)
  {
    cool::basis::binary<Size> result;

    if (length > Size * 8)
      throw exception::illegal_argument("Network mask length exceeds data size");

    std::size_t limit = length >> 3;
    for (std::size_t i = 0; i < limit; ++i)
      result[i] = 0xff;

    std::size_t limit2 = length & 0x07;
    if (limit2 > 0)
    {
      uint8_t aux = 0x80;
      for (int i = 1; i < limit2; ++i)
      {
        aux >>= 1;
        aux |= 0x80;
      }
      result[limit] = aux;
    }
    return result;
  }



} // namespace entrails

} } // namespace

#endif
