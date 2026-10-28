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

usb_raw = extract_sym("Connector.kicad_sym", "USB_C_Receptacle_PowerOnly_6P")
esp_raw = extract_sym("RF_Module.kicad_sym", "ESP32-WROOM-32")
c_raw = extract_sym("Device.kicad_sym", "C")
r_raw = extract_sym("Device.kicad_sym", "R")
sw_raw = extract_sym("Switch.kicad_sym", "SW_Push")

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

def rename_sym(raw_text, old_name, new_name):
    return raw_text.replace(f'(symbol "{old_name}"', f'(symbol "{new_name}"', 1)

usb_sym = "    " + "\n    ".join(rename_sym(usb_raw, "USB_C_Receptacle_PowerOnly_6P", "Connector:USB_C_Receptacle_PowerOnly_6P").splitlines())
esp_sym = "    " + "\n    ".join(rename_sym(esp_raw, "ESP32-WROOM-32", "RF_Module:ESP32-WROOM-32").splitlines())
c_sym = "    " + "\n    ".join(rename_sym(c_raw, "C", "Device:C").splitlines())
r_sym = "    " + "\n    ".join(rename_sym(r_raw, "R", "Device:R").splitlines())
sw_sym = "    " + "\n    ".join(rename_sym(sw_raw, "SW_Push", "Switch:SW_Push").splitlines())

LIB_SYMBOLS = f"""  (lib_symbols
{usb_sym}
{ams_sym}
{esp_sym}
{c_sym}
{r_sym}
{sw_sym}
  )"""

def sym_instance(lib_id, ref, val, x, y, fp="", datasheet="", pins=None, ref_offset=(0, -8), val_offset=(0, -5.5)):
    rx = x + ref_offset[0]
    ry = y + ref_offset[1]
    vx = x + val_offset[0]
    vy = y + val_offset[1]
    
    res = []
    res.append(f'  (symbol (lib_id "{lib_id}") (at {x:.2f} {y:.2f} 0) (unit 1)')
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
    if datasheet:
        res.append(f'    (property "Datasheet" "{datasheet}" (at {x:.2f} {y:.2f} 0)')
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
    
    Y_GND = 142.24
    Y_VBUS = 73.66
    
    body = []
    
    # ── 1. Symbols ────────────────────────────────────────────────────────────
    # J1 (USB-C)
    body.append(sym_instance(
        "Connector:USB_C_Receptacle_PowerOnly_6P", "J1", "USB_C_Receptacle_PowerOnly_6P",
        40.64, 81.28,
        fp="Connector_USB:USB_C_Receptacle_GCT_USB4105-GF-A_1x06_P2.54mm_Vertical",
        datasheet="https://www.gct.co/files/ee/usb4105.pdf",
        pins=["A5", "A9", "A12", "B5", "B9", "B12", "SH"],
        ref_offset=(0, -18), val_offset=(0, -15.5)
    ))
    
    # R1 (5.1k CC1)
    body.append(sym_instance(
        "Device:R", "R1", "5.1k",
        66.04, 101.60,
        fp="Resistor_SMD:R_0402_1005Metric",
        pins=["1", "2"],
        ref_offset=(-4.5, 0), val_offset=(4.5, 0)
    ))
    
    # R2 (5.1k CC2)
    body.append(sym_instance(
        "Device:R", "R2", "5.1k",
        76.20, 101.60,
        fp="Resistor_SMD:R_0402_1005Metric",
        pins=["1", "2"],
        ref_offset=(-4.5, 0), val_offset=(4.5, 0)
    ))
    
    # C1 (10uF VBUS)
    body.append(sym_instance(
        "Device:C", "C1", "10uF",
        88.90, 88.90,
        fp="Capacitor_SMD:C_0805_2012Metric",
        pins=["1", "2"],
        ref_offset=(-4.5, 0), val_offset=(4.5, 0)
    ))
    
    # U2 (AMS1117-3.3)
    body.append(sym_instance(
        "Regulator_Linear:AMS1117-3.3", "U2", "AMS1117-3.3",
        106.68, 73.66,
        fp="Package_TO_SOT_SMD:SOT-223-3_TabPin2",
        datasheet="http://www.advanced-monolithic.com/pdf/ds1117.pdf",
        pins=["1", "2", "3"],
        ref_offset=(0, -6), val_offset=(0, -4)
    ))
    
    # C2 (22uF 3V3)
    body.append(sym_instance(
        "Device:C", "C2", "22uF",
        127.00, 88.90,
        fp="Capacitor_SMD:C_0805_2012Metric",
        pins=["1", "2"],
        ref_offset=(-4.5, 0), val_offset=(4.5, 0)
    ))
    
    # C3 (100nF 3V3)
    body.append(sym_instance(
        "Device:C", "C3", "100nF",
        137.16, 88.90,
        fp="Capacitor_SMD:C_0402_1005Metric",
        pins=["1", "2"],
        ref_offset=(-4.5, 0), val_offset=(4.5, 0)
    ))
    
    # R3 (10k EN pull-up)
    body.append(sym_instance(
        "Device:R", "R3", "10k",
        172.72, 57.15,
        fp="Resistor_SMD:R_0402_1005Metric",
        pins=["1", "2"],
        ref_offset=(-4.5, 0), val_offset=(4.5, 0)
    ))
    
    # C4 (100nF EN reset delay)
    body.append(sym_instance(
        "Device:C", "C4", "100nF",
        172.72, 78.74,
        fp="Capacitor_SMD:C_0402_1005Metric",
        pins=["1", "2"],
        ref_offset=(-4.5, 0), val_offset=(4.5, 0)
    ))
    
    # U1 (ESP32-WROOM-32)
    body.append(sym_instance(
        "RF_Module:ESP32-WROOM-32", "U1", "ESP32-WROOM-32",
        205.74, 96.52,
        fp="RF_Module:ESP32-WROOM-32",
        datasheet="https://www.espressif.com/sites/default/files/documentation/esp32-wroom-32_datasheet_en.pdf",
        pins=[str(i) for i in range(1, 40)],
        ref_offset=(0, -42), val_offset=(0, -39)
    ))
    
    if unit_num == 1:
        # Unit 1: Single SW1 (TOKEN_BUTTON on IO4)
        body.append(sym_instance(
            "Switch:SW_Push", "SW1", "SW_Push",
            274.32, 96.52,
            fp="Button_Switch_THT:SW_Push_1P1T_NO_6x6mm_H9.5mm",
            pins=["1", "2"],
            ref_offset=(0, -5.5), val_offset=(0, -3.5)
        ))
        body.append(text_label("TOKEN_BUTTON (IO4)", 274.32, 102.5, 1.27))
    else:
        # Unit 2: Three buttons (CALL_NEXT, CALL_AGAIN, RESET_QUEUE)
        body.append(sym_instance(
            "Switch:SW_Push", "SW1", "SW_Push",
            274.32, 96.52,
            fp="Button_Switch_THT:SW_Push_1P1T_NO_6x6mm_H9.5mm",
            pins=["1", "2"],
            ref_offset=(0, -5.5), val_offset=(0, -3.5)
        ))
        body.append(text_label("CALL_NEXT (IO15)", 274.32, 102.5, 1.27))
        
        body.append(sym_instance(
            "Switch:SW_Push", "SW2", "SW_Push",
            274.32, 114.30,
            fp="Button_Switch_THT:SW_Push_1P1T_NO_6x6mm_H9.5mm",
            pins=["1", "2"],
            ref_offset=(0, -5.5), val_offset=(0, -3.5)
        ))
        body.append(text_label("CALL_AGAIN (IO13)", 274.32, 120.5, 1.27))
        
        body.append(sym_instance(
            "Switch:SW_Push", "SW3", "SW_Push",
            274.32, 132.08,
            fp="Button_Switch_THT:SW_Push_1P1T_NO_6x6mm_H9.5mm",
            pins=["1", "2"],
            ref_offset=(0, -5.5), val_offset=(0, -3.5)
        ))
        body.append(text_label("RESET_QUEUE (IO14)", 274.32, 138.5, 1.27))
    
    # ── 2. Wires and Junctions ────────────────────────────────────────────────
    # Horizontal bottom GND rail
    X_GND_START = 33.02
    X_GND_END = 284.48
    body.append(wire(X_GND_START, Y_GND, X_GND_END, Y_GND))
    
    # J1 SHIELD to GND
    body.append(wire(33.02, 99.06, 33.02, Y_GND))
    body.append(junction(33.02, Y_GND))
    
    # J1 GND (pin A12) to GND
    body.append(wire(40.64, 99.06, 40.64, Y_GND))
    body.append(junction(40.64, Y_GND))
    
    # VBUS horizontal wire: from J1 Pin A9 (55.88, 73.66) to U2 VI (99.06, 73.66)
    body.append(wire(55.88, 73.66, 88.90, 73.66))
    body.append(wire(88.90, 73.66, 99.06, 73.66))
    
    # J1 CC1 (Pin A5) -> R1
    body.append(wire(55.88, 86.36, 66.04, 86.36))
    body.append(wire(66.04, 86.36, 66.04, 97.79))
    body.append(wire(66.04, 105.41, 66.04, Y_GND))
    body.append(junction(66.04, Y_GND))
    
    # J1 CC2 (Pin B5) -> R2
    body.append(wire(55.88, 88.90, 76.20, 88.90))
    body.append(wire(76.20, 88.90, 76.20, 97.79))
    body.append(wire(76.20, 105.41, 76.20, Y_GND))
    body.append(junction(76.20, Y_GND))
    
    # C1 (10uF) VBUS decoupling
    body.append(wire(88.90, 73.66, 88.90, 85.09))
    body.append(junction(88.90, 73.66))
    body.append(wire(88.90, 92.71, 88.90, Y_GND))
    body.append(junction(88.90, Y_GND))
    
    # U2 GND (pin 1) to GND
    body.append(wire(106.68, 81.28, 106.68, Y_GND))
    body.append(junction(106.68, Y_GND))
    
    # 3.3V VO Rail: from U2 Pin 2 (114.30, 73.66) to C2, C3, and up to ESP32
    body.append(wire(114.30, 73.66, 127.00, 73.66))
    body.append(wire(127.00, 73.66, 137.16, 73.66))
    body.append(wire(137.16, 73.66, 147.32, 73.66))
    
    # C2 (22uF)
    body.append(wire(127.00, 73.66, 127.00, 85.09))
    body.append(junction(127.00, 73.66))
    body.append(wire(127.00, 92.71, 127.00, Y_GND))
    body.append(junction(127.00, Y_GND))
    
    # C3 (100nF)
    body.append(wire(137.16, 73.66, 137.16, 85.09))
    body.append(junction(137.16, 73.66))
    body.append(wire(137.16, 92.71, 137.16, Y_GND))
    body.append(junction(137.16, Y_GND))
    
    # 3.3V rail routing to ESP32 top:
    # Up to Y = 48.26
    body.append(wire(147.32, 73.66, 147.32, 48.26))
    body.append(wire(147.32, 48.26, 172.72, 48.26))
    body.append(wire(172.72, 48.26, 205.74, 48.26))
    # Down to ESP32 VDD Pin 2
    body.append(wire(205.74, 48.26, 205.74, 60.96))
    
    # R3 (10k EN pull-up)
    body.append(wire(172.72, 48.26, 172.72, 53.34))
    body.append(junction(172.72, 48.26))
    body.append(wire(172.72, 60.96, 172.72, 66.04))
    
    # Node at (172.72, 66.04) -> ESP32 EN Pin 3 (190.50, 66.04)
    body.append(wire(172.72, 66.04, 190.50, 66.04))
    body.append(junction(172.72, 66.04))
    
    # C4 (100nF EN capacitor to GND)
    body.append(wire(172.72, 66.04, 172.72, 74.93))
    body.append(wire(172.72, 82.55, 172.72, Y_GND))
    body.append(junction(172.72, Y_GND))
    
    # ESP32 GND (pin 1) to GND
    body.append(wire(205.74, 132.08, 205.74, Y_GND))
    body.append(junction(205.74, Y_GND))
    
    # Switch connections
    if unit_num == 1:
        # SW1 on IO4 (Pin 26: 220.98, 76.20)
        body.append(wire(220.98, 76.20, 243.84, 76.20))
        body.append(wire(243.84, 76.20, 243.84, 96.52))
        body.append(wire(243.84, 96.52, 269.24, 96.52))
        body.append(wire(279.40, 96.52, 284.48, 96.52))
        body.append(wire(284.48, 96.52, 284.48, Y_GND))
        body.append(junction(284.48, Y_GND))
    else:
        # Unit 2 Buttons:
        # SW1: CALL_NEXT on IO15 (Pin 23: 220.98, 88.90)
        body.append(wire(220.98, 88.90, 243.84, 88.90))
        body.append(wire(243.84, 88.90, 243.84, 96.52))
        body.append(wire(243.84, 96.52, 269.24, 96.52))
        
        # SW2: CALL_AGAIN on IO13 (Pin 16: 220.98, 83.82)
        body.append(wire(220.98, 83.82, 251.46, 83.82))
        body.append(wire(251.46, 83.82, 251.46, 114.30))
        body.append(wire(251.46, 114.30, 269.24, 114.30))
        
        # SW3: RESET_QUEUE on IO14 (Pin 13: 220.98, 86.36)
        body.append(wire(220.98, 86.36, 259.08, 86.36))
        body.append(wire(259.08, 86.36, 259.08, 132.08))
        body.append(wire(259.08, 132.08, 269.24, 132.08))
        
        # Right side vertical common GND bus for switches:
        body.append(wire(279.40, 96.52, 284.48, 96.52))
        body.append(wire(279.40, 114.30, 284.48, 114.30))
        body.append(wire(279.40, 132.08, 284.48, 132.08))
        
        body.append(wire(284.48, 96.52, 284.48, 114.30))
        body.append(wire(284.48, 114.30, 284.48, 132.08))
        body.append(wire(284.48, 132.08, 284.48, Y_GND))
        
        body.append(junction(284.48, 114.30))
        body.append(junction(284.48, 132.08))
        body.append(junction(284.48, Y_GND))

    # Assemble full .kicad_sch
    content = [
        '(kicad_sch',
        '  (version 20231120)',
        '  (generator "kicad_sch")',
        '  (paper "A4")',
        '  (title_block',
        f'    (title "{title}")',
        '    (date "2026-10-18")',
        '    (rev "1.0")',
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

print("Done generating schematics!")
