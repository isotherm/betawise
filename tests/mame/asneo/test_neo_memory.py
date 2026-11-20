"""Direct LCD records must preserve orientation, atomicity and exact text."""
import copy
import json
from pathlib import Path
import tempfile
import unittest

from neo_reader import bitmap, decode, lcd_frames
from logo_graphics import compare, expected_image


def controller():
    return dict(ram=bytes(1188).hex(), m_start_line=0, m_display_on=1,
                m_adc=0, m_com_reverse=0, m_reverse=0, m_all_points=0, m_reset=0)


def encode(bits):
    states = [controller(), controller()]
    for side in range(2):
        data = bytearray(1188)
        for y in range(64):
            for col in range(132):
                if bits[y*264+side*132+col]:
                    data[y//8*132+col] |= 1 << (y%8)
        states[side]['ram'] = data.hex()
    return states


class MemoryTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)

    def record(self, states, samples=3):
        path = self.root/'sample.lcd.json'
        path.write_text(json.dumps(dict(format='neo-lcd-v1',
                        samples=[dict(controllers=copy.deepcopy(states)) for _ in range(samples)])))
        return path

    def test_scroll_and_axis_flags(self):
        states = [controller(), controller()]
        ram = bytearray(1188); ram[0]=1
        states[1]['ram']=ram.hex()
        path=self.record(states)
        bits=lcd_frames(path)[0]
        self.assertEqual([i for i,v in enumerate(bits) if v], [132])
        states[1].update(m_start_line=8, m_adc=1, m_com_reverse=1)
        bits=lcd_frames(self.record(states))[0]
        self.assertEqual([i for i,v in enumerate(bits) if v], [7*264+263])

    def test_display_modes(self):
        states=[controller(),controller()]
        states[0]['m_reverse']=1
        self.assertEqual(sum(lcd_frames(self.record(states))[0]),132*64)
        states[0].update(m_reverse=0,m_all_points=1,m_display_on=0)
        self.assertEqual(sum(lcd_frames(self.record(states))[0]),0)
        states[0].update(m_display_on=1,m_reset=1)
        self.assertEqual(sum(lcd_frames(self.record(states))[0]),0)
        states[0].update(m_reset=0)
        self.assertEqual(sum(lcd_frames(self.record(states))[0]),132*64)

    def test_three_identical_samples_required(self):
        path=self.record([controller(),controller()],samples=2)
        with self.assertRaises(ValueError):lcd_frames(path)
        path=self.record([controller(),controller()])
        data=json.loads(path.read_text())
        data['samples'][1]['controllers'][0]['m_start_line']=8
        path.write_text(json.dumps(data))
        with self.assertRaises(ValueError):lcd_frames(path)

    def test_record_validation(self):
        for change in [dict(ram='00'),dict(m_start_line=64),dict(m_adc=2),dict(m_reset=True)]:
            states=[controller(),controller()];states[0].update(change)
            with self.assertRaises(ValueError):lcd_frames(self.record(states))
        path=self.root/'wrong.lcd.json';path.write_text('[]')
        with self.assertRaises(ValueError):lcd_frames(path)

    def test_ram_matches_real_terminal_fixture(self):
        fixture=Path(__file__).parent/'fixtures/backspace-clean.png'
        bits=bitmap(fixture,(264,64))
        path=self.record(encode(bits))
        self.assertEqual(decode(path),decode(fixture))
        self.assertTrue(decode(path).clean)

    def test_graphics_assertion_needs_no_screenshots(self):
        expected=expected_image('square20')
        bits=[0]*(264*64)
        for y in range(64):
            for x in range(132):bits[y*264+x+132]=int(bool(expected.getpixel((x,y))))
        path=self.record(encode(bits))
        result=compare([path],'square20',self.root/'square')
        self.assertEqual(result['foreground_pixels'],80)
        bits[12*264+132+70]=0
        path=self.record(encode(bits))
        with self.assertRaisesRegex(ValueError,'1 wrong pixels'):
            compare([path],'square20',self.root/'bad')


if __name__=='__main__':unittest.main()
