import os
import uuid

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
sym_dir = r"C:\Program Files\KiCad\10.0\share\kicad\symbols"

def extract_sym(filename, sym_name):
    path = os.path.join(sym_dir, filename)
    with open(path, "r", encoding="utf-8") as f:
        content = f.read()
    target = f'(symbol "{sym_name}"'
    idx = content.find(target)
    if idx == -1:
        raise ValueError(f"{sym_name} not found in {filename}")
    depth = 0
    end_idx = -1
    for i in range(idx, len(content)):
        if content[i] == '(':
            depth += 1
        elif content[i] == ')':
            depth -= 1
            if depth == 0:
                end_idx = i + 1
                break
    return content[idx:end_idx]

def rename_sym(raw_text, old_name, new_name):
    return raw_text.replace(f'(symbol "{old_name}"', f'(symbol "{new_name}"', 1)

usb_raw = extract_sym("Connector.kicad_sym", "USB_C_Receptacle_PowerOnly_6P")
esp_raw = extract_sym("RF_Module.kicad_sym", "ESP32-WROOM-32")
c_raw   = extract_sym("Device.kicad_sym", "C")
r_raw   = extract_sym("Device.kicad_sym", "R")
sw_raw  = extract_sym("Switch.kicad_sym", "SW_Push")

ams_sym = """    (symbol "Regulator_Linear:AMS1117-3.3"
      (pin_names (offset 1.016))
      (in_bom yes) (on_board yes)
      (property "Reference" "U" (at -3.81 3.175 0) (effects (font (size 1.27 1.27))))
      (property "Value" "AMS1117-3.3" (at 0 3.175 0) (effects (font (size 1.27 1.27)) (justify left)))
      (property "Footprint" "Package_TO_SOT_SMD:SOT-223-3_TabPin2" (at 0 5.08 0) (effects (font (size 1.27 1.27)) (hide yes)))
      (property "Datasheet" "http://www.advanced-monolithic.com/pdf/ds1117.pdf" (at 2.54 -6.35 0) (effects (font (size 1.27 1.27)) (hide yes)))
      (symbol "AMS1117-3.3_0_1"
        (rectangle (start -5.08 -5.08) (end 5.08 1.905)
          (stroke (width 0.254) (type default))
          (fill (type background))
        )
      )
      (symbol "AMS1117-3.3_1_1"
        (pin power_in line (at 0 -7.62 90) (length 2.54)
          (name "GND" (effects (font (size 1.27 1.27))))
          (number "1" (effects (font (size 1.27 1.27))))
        )
        (pin power_out line (at 7.62 0 180) (length 2.54)
          (name "VO" (effects (font (size 1.27 1.27))))
          (number "2" (effects (font (size 1.27 1.27))))
        )
        (pin power_in line (at -7.62 0 0) (length 2.54)
          (name "VI" (effects (font (size 1.27 1.27))))
          (number "3" (effects (font (size 1.27 1.27))))
        )
      )
    )"""

usb_sym = "    " + "\n    ".join(rename_sym(usb_raw, "USB_C_Receptacle_PowerOnly_6P", "Connector:USB_C_Receptacle_PowerOnly_6P").splitlines())
esp_sym = "    " + "\n    ".join(rename_sym(esp_raw, "ESP32-WROOM-32", "RF_Module:ESP32-WROOM-32").splitlines())
c_sym   = "    " + "\n    ".join(rename_sym(c_raw, "C", "Device:C").splitlines())
r_sym   = "    " + "\n    ".join(rename_sym(r_raw, "R", "Device:R").splitlines())
sw_sym  = "    " + "\n    ".join(rename_sym(sw_raw, "SW_Push", "Switch:SW_Push").splitlines())

LIB_SYMBOLS = f"""  (lib_symbols
{usb_sym}
{ams_sym}
{esp_sym}
{c_sym}
{r_sym}
{sw_sym}
  )"""

def sym_instance(lib_id, ref, val, x, y, rot=0, fp="", pins=None, ref_offset=(0, -6), val_offset=(0, -4)):
    rx = x + ref_offset[0]
    ry = y + ref_offset[1]
    vx = x + val_offset[0]
    vy = y + val_offset[1]
    
    res = []
    res.append(f'  (symbol (lib_id "{lib_id}") (at {x:.2f} {y:.2f} {rot}) (unit 1)')
    res.append('    (in_bom yes) (on_board yes)')
    res.append(f'    (uuid "{uuid.uuid4()}")')
    res.append(f'    (property "Reference" "{ref}" (at {rx:.2f} {ry:.2f} 0)')
    res.append('      (effects (font (size 1.27 1.27)))')
    res.append('    )')
    res.append(f'    (property "Value" "{val}" (at {vx:.2f} {vy:.2f} 0)')
    res.append('      (effects (font (size 1.27 1.27)))')
    res.append('    )')
    if fp:
        res.append(f'    (property "Footprint" "{fp}" (at {x:.2f} {y:.2f} 0)')
        res.append('      (effects (font (size 1.27 1.27)) (hide yes))')
        res.append('    )')
    if pins:
        for p in pins:
            res.append(f'    (pin "{p}" (uuid "{uuid.uuid4()}"))')
    res.append('  )\n')
    return '\n'.join(res)

def wire(x1, y1, x2, y2):
    return (
        f'  (wire (pts (xy {x1:.2f} {y1:.2f}) (xy {x2:.2f} {y2:.2f}))\n'
        f'    (stroke (width 0) (type default))\n'
        f'    (uuid "{uuid.uuid4()}")\n'
        f'  )\n'
    )

def junction(x, y):
    return (
        f'  (junction (at {x:.2f} {y:.2f}) (diameter 0) (color 0 0 0 0)\n'
        f'    (uuid "{uuid.uuid4()}")\n'
        f'  )\n'
    )

def text_label(lbl, x, y, size=1.27):
    return (
        f'  (text "{lbl}" (at {x:.2f} {y:.2f} 0)\n'
        f'    (effects\n'
        f'      (font\n'
        f'        (size {size} {size})\n'
        f'      )\n'
        f'    )\n'
        f'    (uuid "{uuid.uuid4()}")\n'
        f'  )\n'
    )

def generate_schematic(unit_num):
    title = f"TMS Unit {unit_num} - {'Token Dispenser' if unit_num == 1 else 'Display / Counter Unit'}"
    subtitle = "Token Dispenser" if unit_num == 1 else "Display/Counter Unit (Next, Call Again, Reset)"
    
    body = []
    Y_GND = 145.00
    
    # ── 1. USB-C 6P POWER INPUT ───────────────────────────────────────────────
    # J1 at (35.00, 75.00)
    body.append(sym_instance(
        "Connector:USB_C_Receptacle_PowerOnly_6P", "J1", "USB_C_Receptacle_PowerOnly_6P",
        35.00, 75.00,
        fp="Connector_USB:USB_C_Receptacle_GCT_USB4105-GF-A_1x06_P2.54mm_Vertical",
        pins=["A5", "A9", "A12", "B5", "B9", "B12", "SH"],
        ref_offset=(0, -18), val_offset=(0, -15.5)
    ))
    
    # J1 SHIELD & GND to bottom GND
    body.append(wire(27.38, 92.78, 27.38, Y_GND))
    body.append(wire(35.00, 92.78, 35.00, Y_GND))
    body.append(junction(35.00, Y_GND))
    
    # CC1 (50.24, 80.08) -> R1 (5.1k) at (57.00, 95.00) -> GND
    body.append(wire(50.24, 80.08, 57.00, 80.08))
    body.append(wire(57.00, 80.08, 57.00, 91.19))
    body.append(sym_instance("Device:R", "R1", "5.1k", 57.00, 95.00, pins=["1", "2"], ref_offset=(-4.5, 0), val_offset=(4.5, 0)))
    body.append(wire(57.00, 98.81, 57.00, Y_GND))
    body.append(junction(57.00, Y_GND))
    
    # CC2 (50.24, 82.62) -> R2 (5.1k) at (65.00, 95.00) -> GND
    body.append(wire(50.24, 82.62, 65.00, 82.62))
    body.append(wire(65.00, 82.62, 65.00, 91.19))
    body.append(sym_instance("Device:R", "R2", "5.1k", 65.00, 95.00, pins=["1", "2"], ref_offset=(-4.5, 0), val_offset=(4.5, 0)))
    body.append(wire(65.00, 98.81, 65.00, Y_GND))
    body.append(junction(65.00, Y_GND))
    
    # ── 2. VOLTAGE REGULATOR AMS1117-3.3 ──────────────────────────────────────
    # VBUS wire (Y = 67.38)
    body.append(wire(50.24, 67.38, 77.00, 67.38))
    
    # C1 (10uF Input Decoupling) at (77.00, 82.00)
    body.append(junction(77.00, 67.38))
    body.append(wire(77.00, 67.38, 77.00, 78.19))
    body.append(sym_instance("Device:C", "C1", "10uF", 77.00, 82.00, pins=["1", "2"], ref_offset=(-4.5, 0), val_offset=(4.5, 0)))
    body.append(wire(77.00, 85.81, 77.00, Y_GND))
    body.append(junction(77.00, Y_GND))
    
    # U2: AMS1117-3.3 at (95.00, 67.38)
    body.append(wire(77.00, 67.38, 87.38, 67.38)) # VI pin 3
    body.append(sym_instance("Regulator_Linear:AMS1117-3.3", "U2", "AMS1117-3.3", 95.00, 67.38, pins=["1", "2", "3"], ref_offset=(0, -6), val_offset=(0, -4)))
    # U2 GND pin 1 (95.00, 75.00) -> GND
    body.append(wire(95.00, 75.00, 95.00, Y_GND))
    body.append(junction(95.00, Y_GND))
    
    # VO pin 2 (102.62, 67.38) -> 3.3V rail
    # C2 (22uF Output Bulk) at (113.00, 82.00)
    body.append(wire(102.62, 67.38, 113.00, 67.38))
    body.append(junction(113.00, 67.38))
    body.append(wire(113.00, 67.38, 113.00, 78.19))
    body.append(sym_instance("Device:C", "C2", "22uF", 113.00, 82.00, pins=["1", "2"], ref_offset=(-4.5, 0), val_offset=(4.5, 0)))
    body.append(wire(113.00, 85.81, 113.00, Y_GND))
    body.append(junction(113.00, Y_GND))
    
    # C3 (100nF Bypass) at (121.00, 82.00)
    body.append(wire(113.00, 67.38, 121.00, 67.38))
    body.append(junction(121.00, 67.38))
    body.append(wire(121.00, 67.38, 121.00, 78.19))
    body.append(sym_instance("Device:C", "C3", "100nF", 121.00, 82.00, pins=["1", "2"], ref_offset=(-4.5, 0), val_offset=(4.5, 0)))
    body.append(wire(121.00, 85.81, 121.00, Y_GND))
    body.append(junction(121.00, Y_GND))
    
    # ── 3. 3.3V POWER BUS TO ESP32 & PULL-UPS ─────────────────────────────────
    # 3.3V wire goes up to Y = 48.00 and runs across the top of the circuit
    body.append(wire(121.00, 67.38, 129.00, 67.38))
    body.append(wire(129.00, 67.38, 129.00, 48.00))
    body.append(wire(129.00, 48.00, 150.00, 48.00))
    body.append(junction(150.00, 48.00))
    body.append(wire(150.00, 48.00, 185.00, 48.00))
    body.append(junction(185.00, 48.00))
    body.append(wire(185.00, 48.00, 215.00, 48.00))
    
    # ── 4. RESET SUBSYSTEM (EN Pin 3: 169.76, 64.52) ──────────────────────────
    # R3 (10k EN pull-up) at (150.00, 56.00)
    body.append(wire(150.00, 48.00, 150.00, 52.19))
    body.append(sym_instance("Device:R", "R3", "10k", 150.00, 56.00, pins=["1", "2"], ref_offset=(-4.5, 0), val_offset=(4.5, 0)))
    body.append(wire(150.00, 59.81, 150.00, 64.52))
    body.append(junction(150.00, 64.52))
    
    # EN wire to ESP32 Pin 3 (169.76, 64.52)
    body.append(wire(150.00, 64.52, 169.76, 64.52))
    
    # C4 (100nF reset delay cap) at (150.00, 75.00)
    body.append(wire(150.00, 64.52, 150.00, 71.19))
    body.append(sym_instance("Device:C", "C4", "100nF", 150.00, 75.00, pins=["1", "2"], ref_offset=(-4.5, 0), val_offset=(4.5, 0)))
    body.append(wire(150.00, 78.81, 150.00, Y_GND))
    body.append(junction(150.00, Y_GND))
    
    # ── 5. ESP32 MICROCONTROLLER U1 ───────────────────────────────────────────
    # U1 at (185.00, 95.00)
    body.append(sym_instance(
        "RF_Module:ESP32-WROOM-32", "U1", "ESP32-WROOM-32",
        185.00, 95.00,
        fp="RF_Module:ESP32-WROOM-32",
        pins=[str(i) for i in range(1, 40)],
        ref_offset=(0, -39), val_offset=(0, -36)
    ))
    
    # VDD Pin 2 (185.00, 59.44) -> 3.3V top wire at (185.00, 48.00)
    body.append(wire(185.00, 48.00, 185.00, 59.44))
    
    # GND Pin 1 (185.00, 130.56) -> bottom GND
    body.append(wire(185.00, 130.56, 185.00, Y_GND))
    body.append(junction(185.00, Y_GND))
    
    # ── 6. NORMAL BOOT PULL-UP (IO0 Pin 25: 200.24, 64.52) ───────────────────
    # R4 (10k IO0 pull-up to 3.3V for SPI Flash Boot Mode) at (215.00, 56.00)
    body.append(wire(215.00, 48.00, 215.00, 52.19))
    body.append(sym_instance("Device:R", "R4", "10k", 215.00, 56.00, pins=["1", "2"], ref_offset=(-4.5, 0), val_offset=(4.5, 0)))
    body.append(wire(215.00, 59.81, 215.00, 64.52))
    # IO0 wire from ESP32 Pin 25 (200.24, 64.52) to R4 bottom
    body.append(wire(200.24, 64.52, 215.00, 64.52))
    
    # ── 7. OPERATOR PUSHBUTTONS WITH HARDWARE DEBOUNCING ──────────────────────
    if unit_num == 1:
        # Unit 1: Token Button on GPIO4 (Pin 26: 200.24, 74.68)
        Y_BTN = 95.00
        body.append(wire(200.24, 74.68, 215.00, 74.68))
        body.append(wire(215.00, 74.68, 215.00, Y_BTN))
        body.append(junction(215.00, Y_BTN))
        body.append(wire(215.00, Y_BTN, 234.92, Y_BTN)) # to SW1 Pin 1
        
        # Debounce capacitor C_DB1 (100nF) at (215.00, 110.00)
        body.append(wire(215.00, Y_BTN, 215.00, 106.19))
        body.append(sym_instance("Device:C", "C_DB1", "100nF", 215.00, 110.00, pins=["1", "2"], ref_offset=(-5, 0), val_offset=(5, 0)))
        body.append(wire(215.00, 113.81, 215.00, Y_GND))
        body.append(junction(215.00, Y_GND))
        
        # SW1: TOKEN_BTN at (240.00, Y_BTN)
        body.append(sym_instance("Switch:SW_Push", "SW1", "TOKEN_BTN", 240.00, Y_BTN, pins=["1", "2"], ref_offset=(0, -6), val_offset=(0, -3.5)))
        body.append(wire(245.08, Y_BTN, 255.00, Y_BTN))
        body.append(wire(255.00, Y_BTN, 255.00, Y_GND))
        body.append(junction(255.00, Y_GND))
        
        body.append(text_label("TOKEN BUTTON (GPIO4)", 215.00, 85.00, 1.3))
        body.append(text_label("Short: Generate Token | 10s: WiFi Reset", 215.00, 90.00, 1.1))
        
        # Bottom GND rail
        body.append(wire(27.38, Y_GND, 255.00, Y_GND))
        
    else:
        # Unit 2: Three operator buttons with debouncing capacitors
        # 1. SW1: NEXT on IO15 (Pin 23: 200.24, 87.38)
        Y_BTN1 = 80.00
        body.append(wire(200.24, 87.38, 210.00, 87.38))
        body.append(wire(210.00, 87.38, 210.00, Y_BTN1))
        body.append(wire(210.00, Y_BTN1, 218.00, Y_BTN1))
        body.append(junction(218.00, Y_BTN1))
        body.append(wire(218.00, Y_BTN1, 234.92, Y_BTN1))
        
        # Debounce C_DB1 (100nF) at (218.00, 88.00)
        body.append(wire(218.00, Y_BTN1, 218.00, 84.19))
        body.append(sym_instance("Device:C", "C_DB1", "100nF", 218.00, 88.00, pins=["1", "2"], ref_offset=(-5, 0), val_offset=(5, 0)))
        body.append(wire(218.00, 91.81, 218.00, 95.00))
        body.append(wire(218.00, 95.00, 255.00, 95.00))
        
        # SW1
        body.append(sym_instance("Switch:SW_Push", "SW1", "NEXT", 240.00, Y_BTN1, pins=["1", "2"], ref_offset=(0, -5), val_offset=(0, -3)))
        body.append(wire(245.08, Y_BTN1, 255.00, Y_BTN1))
        body.append(text_label("NEXT (GPIO15)", 218.00, 75.00, 1.1))
        
        # 2. SW2: CALL AGAIN on IO13 (Pin 16: 200.24, 82.30)
        Y_BTN2 = 105.00
        body.append(wire(200.24, 82.30, 205.00, 82.30))
        body.append(wire(205.00, 82.30, 205.00, Y_BTN2))
        body.append(wire(205.00, Y_BTN2, 218.00, Y_BTN2))
        body.append(junction(218.00, Y_BTN2))
        body.append(wire(218.00, Y_BTN2, 234.92, Y_BTN2))
        
        # Debounce C_DB2 (100nF) at (218.00, 113.00)
        body.append(wire(218.00, Y_BTN2, 218.00, 109.19))
        body.append(sym_instance("Device:C", "C_DB2", "100nF", 218.00, 113.00, pins=["1", "2"], ref_offset=(-5, 0), val_offset=(5, 0)))
        body.append(wire(218.00, 116.81, 218.00, 120.00))
        body.append(wire(218.00, 120.00, 255.00, 120.00))
        
        # SW2
        body.append(sym_instance("Switch:SW_Push", "SW2", "CALL_AGAIN", 240.00, Y_BTN2, pins=["1", "2"], ref_offset=(0, -5), val_offset=(0, -3)))
        body.append(wire(245.08, Y_BTN2, 255.00, Y_BTN2))
        body.append(text_label("CALL AGAIN (GPIO13)", 218.00, 100.00, 1.1))
        
        # 3. SW3: RESET QUEUE on IO14 (Pin 13: 200.24, 84.84)
        Y_BTN3 = 130.00
        body.append(wire(200.24, 84.84, 202.00, 84.84))
        body.append(wire(202.00, 84.84, 202.00, Y_BTN3))
        body.append(wire(202.00, Y_BTN3, 218.00, Y_BTN3))
        body.append(junction(218.00, Y_BTN3))
        body.append(wire(218.00, Y_BTN3, 234.92, Y_BTN3))
        
        # Debounce C_DB3 (100nF) at (218.00, 138.00)
        body.append(wire(218.00, Y_BTN3, 218.00, 134.19))
        body.append(sym_instance("Device:C", "C_DB3", "100nF", 218.00, 138.00, pins=["1", "2"], ref_offset=(-5, 0), val_offset=(5, 0)))
        body.append(wire(218.00, 141.81, 218.00, 142.00))
        body.append(wire(218.00, 142.00, 255.00, 142.00))
        
        # SW3
        body.append(sym_instance("Switch:SW_Push", "SW3", "RESET_QUEUE", 240.00, Y_BTN3, pins=["1", "2"], ref_offset=(0, -5), val_offset=(0, -3)))
        body.append(wire(245.08, Y_BTN3, 255.00, Y_BTN3))
        body.append(text_label("RESET QUEUE (GPIO14) - 5s Hold", 218.00, 125.00, 1.1))
        
        # Vertical GND bus on right side
        body.append(wire(255.00, 80.00, 255.00, 95.00))
        body.append(junction(255.00, 95.00))
        body.append(wire(255.00, 95.00, 255.00, 105.00))
        body.append(junction(255.00, 105.00))
        body.append(wire(255.00, 105.00, 255.00, 120.00))
        body.append(junction(255.00, 120.00))
        body.append(wire(255.00, 120.00, 255.00, 130.00))
        body.append(junction(255.00, 130.00))
        body.append(wire(255.00, 130.00, 255.00, 142.00))
        body.append(junction(255.00, 142.00))
        body.append(wire(255.00, 142.00, 255.00, Y_GND))
        body.append(junction(255.00, Y_GND))
        
        # Bottom GND rail
        body.append(wire(27.38, Y_GND, 255.00, Y_GND))
    
    # Title Block & Lib Symbols
    content = [
        '(kicad_sch',
        '  (version 20231120)',
        '  (generator "kicad_sch")',
        '  (paper "A4")',
        '  (title_block',
        f'    (title "{title}")',
        '    (date "2026-10-18")',
        '    (rev "2.0")',
        '    (company "GTM4Health")',
        '    (comment 1 "USB-C 6P | AMS1117-3.3 | ESP32-WROOM-32")',
        f'    (comment 2 "{subtitle}")',
        '  )',
        LIB_SYMBOLS,
    ]
    content.extend(body)
    content.append(')\n')
    return '\n'.join(content)

# Generate Unit 1
u1_content = generate_schematic(1)
path1 = os.path.join(BASE_DIR, "TMS_Unit1_TokenDispenser.kicad_sch")
with open(path1, "w", encoding="utf-8") as f:
    f.write(u1_content)
print(f"Generated Unit 1: {path1} ({len(u1_content)} bytes)")

# Generate Unit 2
u2_content = generate_schematic(2)
path2 = os.path.join(BASE_DIR, "TMS_Unit2_DisplayCounter.kicad_sch")
with open(path2, "w", encoding="utf-8") as f:
    f.write(u2_content)
print(f"Generated Unit 2: {path2} ({len(u2_content)} bytes)")

print("Done generating updated schematics!")
