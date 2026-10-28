import os
import uuid

# Load official symbols from KiCad installation
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

# For AMS1117-3.3: build a self-contained symbol with the exact yellow rectangle and pins
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

# In official symbols, rename their top-level name to include library prefix
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

print("LIB_SYMBOLS assembled, size:", len(LIB_SYMBOLS))
