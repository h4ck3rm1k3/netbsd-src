// 2002-06-28 pme

// Copyright (C) 2002, 2004, 2005 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along
// with this library; see the file COPYING.  If not, write to the Free
// Software Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
// USA.

// 23.2.3 container adaptros

#include <queue>
#include <testsuite_hooks.h>

// libstdc++/7161
void
test03()
{
  int data[] = {1, 2, 3};
  std::priority_queue<int> pq;
  std::size_t size = pq.size();

  for (int i = 0; i < 3; ++i)
    pq.push(data[i]);

  size = pq.size();
  pq.top();
  for (int i = 0; i < 2; ++i)
    pq.pop();

  while (!pq.empty())
    pq.pop();
}

int main()
{
  test03();
  return 0;
}
