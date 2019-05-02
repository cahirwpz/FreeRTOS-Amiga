#!/usr/bin/env python3

import asyncio
import signal

from prompt_toolkit import prompt
from prompt_toolkit.eventloop import use_asyncio_event_loop
from prompt_toolkit.patch_stdout import patch_stdout
from prompt_toolkit.shortcuts import PromptSession
from prompt_toolkit.history import InMemoryHistory


class Registers():
    names = ['D0', 'D1', 'D2', 'D3', 'D4', 'D5', 'D6', 'D7',
             'A0', 'A1', 'A2', 'A3', 'A4', 'A5', 'A6', 'A7',
             'PC', 'USP', 'ISP', 'SR']

    def __init__(self, **kwargs):
        self._regs = {}
        for n in self.names:
            self._regs[n] = kwargs.get(n, 0)

    def __getitem__(self, name):
        return self._regs[name]

    def __setitem__(self, name, value):
        self._regs[name] = int(value)

    def as_hex(self, name):
        val = self._regs.get(name, None)
        if val is None:
            return 'XXXXXXXX'
        if name == 'SR':
            return '{:04X}'.format(val)
        return '{:08X}'.format(val)

    def __repr__(self):
        regs = ['{}={:08X}'.format(n, self[n]) for n in self.names[:19]]
        regs.append('SR={:04X}'.format(self['SR']))
        return '{' + ' '.join(regs) + '}'


class UaeCommandsMixin():
    @staticmethod
    def _parse_cpu_state(lines):
        # D0 000424B9   D1 00000000   D2 00000000   D3 00000000
        # D4 00000000   D5 00000000   D6 FFFFFFFF   D7 00000000
        # A0 00CF6D1C   A1 00DC0000   A2 00D40000   A3 00000000
        # A4 00D00000   A5 00FC0208   A6 00C00276   A7 00040000
        # USP  00000000 ISP  00040000
        # T=00 S=1 M=0 X=0 N=0 Z=1 V=0 C=0 IMASK=7 STP=0
        # Prefetch fffc (ILLEGAL) 51c8 (DBcc) Chip latch 00000000
        # 00FC0610 51c8 fffc                DBF .W D0,#$fffc == $00fc060e (F)
        # Next PC: 00fc0614
        lines = [line.strip() for line in lines]
        if False:
            for line in lines:
                print(line)
        regs = Registers()
        for l in lines[:5]:
            l = l.split()
            for n, v in zip(l[0::2], l[1::2]):
                regs[n] = int(v, 16)
        sr = lines[5].split()
        T, S, M, X, N, Z, V, C, IMASK, STP = [f.split('=')[1] for f in sr]
        SR_HI = '{:02b}{}{}0{:03b}'.format(int(T), S, M, int(IMASK))
        SR_LO = '000{}{}{}{}{}'.format(X, N, Z, V, C)
        regs['SR'] = int(SR_HI + SR_LO, 2)
        regs['PC'] = int(lines[7].split()[0], 16)
        return regs

    def resume(self, addr=None):
        # {g [<address>]} Start execution at the current address or <address>.
        cmd = 'g'
        if addr:
            cmd += ' ' + hex(addr)
        self.send(cmd)

    def step(self, insn=None):
        # {t [<instructions>]} Step one or more <instructions>.
        cmd = 't'
        if insn:
            cmd +=  ' ' + str(insn)
        self.send(cmd)

    async def read_memory(self, addr, length):
        # {m <address> [<lines>]} Memory dump starting at <address>.
        lines = await self.communicate('m %x %d' % (addr, (length + 15) / 16))
        # 00000004 00C0 0276 00FC 0818 00FC 081A 00FC 081C  ...v............'
        # 00000014 00FC 081E 00FC 0820 00FC 0822 00FC 090E  ....... ..."....'
        # ...
        if False:
            for line in lines:
                print(line)
        hexlines = [''.join(line.strip().split()[1:9]) for line in lines]
        return ''.join(hexlines)[:length*2]

    async def read_long(self, addr):
        longword = await self.read_memory(addr, 4)
        return int(longword, 16)

    async def write_memory(self, addr, data):
        # {W <address> <values[.x] separated by space>} Write into Amiga memory.
        # Assume _data_ is a string of hexadecimal digits.
        hexbytes = []
        while data:
            byte, data = data[:2], data[2:]
            hexbytes.append(byte)
        await self.communicate('W ' + ' '.join(hexbytes))

    async def read_registers(self):
        # {r} Dump state of the CPU.
        return self._parse_cpu_state(await self.communicate('r'))

    async def write_register(self, regname, value):
        # {r <reg> <value>} Modify CPU registers (Dx,Ax,USP,ISP,VBR,...).
        await self.communicate('r {} {:x}'.format(regname, value))

    async def insert_hwbreak(self, addr):
        # {f <address>} Add/remove breakpoint.
        lines = await self.communicate('f %X' % addr)
        assert lines and lines[0] == 'Breakpoint added'

    async def remove_hwbreak(self, addr):
        # {f <address>} Add/remove breakpoint.
        lines = await self.communicate('f %X' % addr)
        assert lines and lines[0] == 'Breakpoint removed'

    async def entry_point(self):
        # assume for now that VBR is at 0
        vbr = 0
        magic = await self.read_long(0)
        if magic != 0x1ee7c0de:
            return None
        bootdata = await self.read_long(4)
        return await self.read_long(bootdata)

    async def prologue(self):
        lines = await self.recv()
        data = {}
        # Breakpoint at 00C04EB0
        if lines[0].startswith('Breakpoint'):
            line = lines.pop(0)
            data['break'] = int(line.split()[2], 16)
        # Exception 27, PC=00C15AC6
        if lines[0].startswith('Exception'):
            line = lines.pop(0)
            data['exception'] = int(line[10:].split(',')[0])
        # just processor state
        data['regs'] = self._parse_cpu_state(lines)
        return data

    async def kill(self):
        # {q} Quit the emulator.
        self.send('q')


async def UaeDebugger(uaedbg):
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
                    uaedbg.send(cmd)
                except EOFError:
                    uaedbg.resume()
                except KeyboardInterrupt:
                    uaedbg.kill()
                lines = await uaedbg.recv()
        except asyncio.CancelledError:
            pass
        except Exception as ex:
            logging.exception('Debugger bug!')
    print('Quitting...')


class UaeProcess(UaeCommandsMixin):
    def __init__(self, proc):
        self.proc = proc

    @property
    def reader(self):
        return self.proc.stderr

    @property
    def writer(self):
        return self.proc.stdin

    def interrupt(self):
        self.proc.send_signal(signal.SIGINT)

    def terminate(self):
        self.proc.send_signal(signal.SIGKILL)

    async def wait(self):
        return await self.proc.wait()

    async def communicate(self, cmd):
        self.send(cmd)
        return await self.recv()

    def send(self, cmd):
        self.writer.write(cmd.encode() + b'\n')

    async def recv(self):
        text = ''

        while True:
            try:
                raw_text = await self.reader.readuntil(b'>')
            except asyncio.streams.IncompleteReadError as ex:
                raise EOFError
            text += raw_text.decode()
            # finished by debugger prompt ?
            if text.endswith('\n>'):
                text = text[:-2]
                return [line.rstrip() for line in text.splitlines()]
