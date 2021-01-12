/*******************************************************************************
 * CLI - A simple command line interface.
 * Copyright (C) 2016-2021 Daniele Pallastrelli
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************/

#ifndef CLI_DETAIL_LINUXKEYBOARD_H_
#define CLI_DETAIL_LINUXKEYBOARD_H_

#include <thread>
#include <memory>
#include <atomic>

#include <cstdio>
#include <termios.h>
#include <unistd.h>

#include "inputdevice.h"


namespace cli
{
namespace detail
{

class LinuxKeyboard : public InputDevice
{
public:
    explicit LinuxKeyboard(Scheduler& _scheduler) :
        InputDevice(_scheduler)
    {
        ToManualMode();
        servant = std::make_unique<std::thread>( [this](){ Read(); } );
        servant->detach();
    }
    ~LinuxKeyboard()
    {
        run = false;
        ToStandardMode();
    }

private:

    void Read()
    {
        while ( run )
        {
            auto k = Get();
            Notify(k);
        }
    }

    std::pair<KeyType,char> Get()
    {
        int ch = std::getchar();
        switch( ch )
        {
            case EOF:
            case 4:  // EOT
                return std::make_pair(KeyType::eof,' ');
                break;
            case 127: return std::make_pair(KeyType::backspace,' '); break;
            case 10: return std::make_pair(KeyType::ret,' '); break;
            case 27: // symbol
                ch = std::getchar();
                if ( ch == 91 ) // arrow keys
                {
                    ch = std::getchar();
                    switch( ch )
                    {
                        case 51:
                            ch = std::getchar();
                            if ( ch == 126 ) return std::make_pair(KeyType::canc,' ');
                            else return std::make_pair(KeyType::ignored,' ');
                            break;
                        case 65: return std::make_pair(KeyType::up,' ');
                        case 66: return std::make_pair(KeyType::down,' ');
                        case 68: return std::make_pair(KeyType::left,' ');
                        case 67: return std::make_pair(KeyType::right,' ');
                        case 70: return std::make_pair(KeyType::end,' ');
                        case 72: return std::make_pair(KeyType::home,' ');
                        default: return std::make_pair(KeyType::ignored,' ');
                    }
                }
                break;
            default: // ascii
            {
                const char c = static_cast<char>(ch);
                return std::make_pair(KeyType::ascii,c);
            }
        }
        return std::make_pair(KeyType::ignored,' ');
    }

    void ToManualMode()
    {
        tcgetattr( STDIN_FILENO, &oldt );
        newt = oldt;
        newt.c_lflag &= ~( (tcflag_t)ICANON | (tcflag_t)ECHO );
        tcsetattr( STDIN_FILENO, TCSANOW, &newt );
    }
    void ToStandardMode()
    {
        tcsetattr( STDIN_FILENO, TCSANOW, &oldt );
    }

    termios oldt;
    termios newt;
    std::atomic<bool> run{ true };
    std::unique_ptr<std::thread> servant;
};

} // namespace detail
} // namespace cli

#endif // CLI_DETAIL_LINUXKEYBOARD_H_

