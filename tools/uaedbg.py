#!/usr/bin/env python3

import argparse
import asyncio
import asyncio.subprocess
import logging
import signal
import sys

from prompt_toolkit.eventloop import use_asyncio_event_loop
from debug.uae import UaeDebugger, UaeProcess
from debug.gdb import GdbConnection, GdbStub


async def UaeLaunch(loop, args):
    # Create the subprocess, redirect the standard I/O to respective pipes
    uaeproc = UaeProcess(
            await asyncio.create_subprocess_exec(
                args.emulator, *args.params,
                stdin=asyncio.subprocess.PIPE,
                stdout=asyncio.subprocess.PIPE,
                stderr=asyncio.subprocess.PIPE))

    # Call FS-UAE debugger on CTRL+C
    # loop.add_signal_handler(signal.SIGINT, uaeproc.interrupt)
    loop.add_signal_handler(signal.SIGINT, uaeproc.terminate)

    # prompt_task = asyncio.ensure_future(UaeDebugger(uaeproc))

    async def GdbClient(reader, writer):
        try:
            await GdbStub(GdbConnection(reader, writer), uaeproc).run()
        except Exception as ex:
            print(ex)

    gdbserver = await asyncio.start_server(
            GdbClient, host='127.0.0.1', port=8888)

    await uaeproc.wait()

    gdbserver.close()


if __name__ == '__main__':
    # Tell prompt_toolkit to use asyncio for the event loop.
    use_asyncio_event_loop()

    logging.basicConfig(level=logging.INFO,
                        format='%(levelname)s: %(message)s')
    # logging.getLogger("asyncio").setLevel(logging.DEBUG)

    if sys.platform == 'win32':
        loop = asyncio.ProactorEventLoop()
        asyncio.set_event_loop(loop)
    else:
        loop = asyncio.get_event_loop()
    # loop.set_debug(True)

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
