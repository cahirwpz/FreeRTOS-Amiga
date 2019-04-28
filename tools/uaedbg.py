#!/usr/bin/env python3

import argparse
import asyncio
import asyncio.subprocess
import logging
import signal
import sys

from prompt_toolkit import prompt
from prompt_toolkit.eventloop import use_asyncio_event_loop
from prompt_toolkit.patch_stdout import patch_stdout
from prompt_toolkit.shortcuts import PromptSession
from prompt_toolkit.history import InMemoryHistory


async def UaeDebuggerPrompt(uaedbg):
    history = InMemoryHistory()
    session = PromptSession('(debug) ', history=history)
    with patch_stdout():
        try:
            lines = await uaedbg.recv()
            while lines is not None:
                for line in lines:
                    print(line)
                try:
                    cmd = ''
                    while not cmd:
                        cmd = await session.prompt(async_=True)
                        cmd.strip()
                    await uaedbg.send(cmd, response=False)
                except EOFError:
                    await uaedbg.send('g', response=False)
                except KeyboardInterrupt:
                    await uaedbg.send('q', response=False)
                lines = await uaedbg.recv()
        except asyncio.CancelledError:
            pass
        except Exception as ex:
            logging.exception('Debugger bug!')
    print('Quitting...')


class UaeDebugger():
    def __init__(self, reader, writer):
        self.reader = reader
        self.writer = writer

    async def send(self, cmd, response=True):
        self.writer.write(cmd.encode() + b'\n')
        if response:
            return await self.recv()

    async def recv(self):
        text = ''

        while True:
            try:
                raw_text = await self.reader.readuntil(b'>')
            except asyncio.streams.IncompleteReadError:
                # end of file encountered ?
                return None
            text += raw_text.decode()
            # finished by debugger prompt ?
            if text.endswith('\n>'):
                text = text[:-2]
                break

        return [line.strip() for line in text.splitlines()]


async def UaeLaunch(loop, args):
    # Create the subprocess, redirect the standard I/O to respective pipes
    uaeproc = await asyncio.create_subprocess_exec(
            args.emulator, *args.params,
            stdin=asyncio.subprocess.PIPE,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE)

    # Call FS-UAE debugger on CTRL+C
    loop.add_signal_handler(signal.SIGINT,
                            lambda: uaeproc.send_signal(signal.SIGINT))

    uaedbg = UaeDebugger(uaeproc.stderr, uaeproc.stdin)

    prompt_task = asyncio.ensure_future(UaeDebuggerPrompt(uaedbg))

    await uaeproc.wait()


if __name__ == '__main__':
    # Tell prompt_toolkit to use asyncio for the event loop.
    use_asyncio_event_loop()

    logging.basicConfig(level=logging.INFO,
                        format='%(levelname)s: %(message)s')

    if sys.platform == 'win32':
        loop = asyncio.ProactorEventLoop()
        asyncio.set_event_loop(loop)
    else:
        loop = asyncio.get_event_loop()

    parser = argparse.ArgumentParser(
        description='Run FS-UAE with enabled console debugger.')
    parser.add_argument('-e', '--emulator', type=str, default='fs-uae',
                        help='Path to FS-UAE emulator binary.')
    parser.add_argument('params', nargs='*', type=str,
                        help='Parameters passed to FS-UAE emulator.')
    args = parser.parse_args()

    uae = UaeLaunch(loop, args)
    loop.run_until_complete(uae)
    loop.close()
