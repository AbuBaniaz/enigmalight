/*
 * EnigmaLight (c) 2014 Speedy1985, Oktay Oeztueter (Based on Boblight from Bob Loosen)
 * parts of this code were taken from
 *
 * - aiograb		(http://schwerkraft.elitedvb.net/projects/aio-grab/)
 * - Boblight (c) 2009 Bob Loosen
 * 
 * EnigmaLight is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * EnigmaLight is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <time.h>
#include <sys/time.h>
#include <assert.h>
#include "Util/Condition.h"
#include "Config.h"

CCondition::CCondition()
{
  pthread_cond_init(&m_cond, NULL);
}

CCondition::~CCondition()
{
  pthread_cond_destroy(&m_cond);
}

void CCondition::Signal()
{
  pthread_cond_signal(&m_cond);
}

void CCondition::Broadcast()
{
  pthread_cond_broadcast(&m_cond);
}

//should have locked before getting here
bool CCondition::Wait(int64_t usecs /*= -1*/)
{
  assert(m_refcount > 0); //if refcount is 0, then the mutex is not locked

  //our mutex is created with PTHREAD_MUTEX_RECURSIVE, which doesn't work with condition variables
  //if the mutex is locked multiple times
  //we keep a refcount, then we unlock enough times so that the mutex is locked only once
  int refcount = m_refcount;
  while (m_refcount > 1)
    Unlock();

  int result = 0;
  if (usecs < 0)
  {
    //pthread_cond_wait will unlock and lock the mutex
    m_refcount--;
    pthread_cond_wait(&m_cond, &m_mutex);
    m_refcount++;
  }
  else
  {
    //get the current time
    struct timespec currtime;
    clock_gettime(CLOCK_REALTIME, &currtime);

    //add the number of microseconds

    currtime.tv_sec += usecs / 1000000;
    currtime.tv_nsec += (usecs % 1000000) * 1000;
    
    if (currtime.tv_nsec >= 1000000000)
    {
      currtime.tv_sec += currtime.tv_nsec / 1000000000;
      currtime.tv_nsec %= 1000000000;
    }

    //pthread_cond_timedwait will unlock and lock the mutex
    m_refcount--;
    result = pthread_cond_timedwait(&m_cond, &m_mutex, &currtime);
    m_refcount++;
  }

  while (m_refcount < refcount)
    Lock();

  return result == 0;
}
