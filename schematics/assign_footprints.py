import os
import shutil
import re

fp_base = r"C:\Program Files\KiCad\10.0\share\kicad\footprints"

def check_footprint_exists(fp_str):
    if not fp_str or ":" not in fp_str:
        return False
    lib, fp = fp_str.split(":", 1)
    p = os.path.join(fp_base, lib + ".pretty", fp + ".kicad_mod")
    return os.path.exists(p)

ASSIGNMENTS = {
    # Unit 1
    ("unit 1 tms.kicad_sch", "U1"): "RF_Module:ESP32-WROOM-32",
    ("unit 1 tms.kicad_sch", "U2"): "Package_TO_SOT_SMD:SOT-223-3_TabPin2",
    ("unit 1 tms.kicad_sch", "J1"): "Connector_USB:USB_C_Receptacle_GCT_USB4125-xx-x_6P_TopMnt_Horizontal",
    ("unit 1 tms.kicad_sch", "R1"): "Resistor_SMD:R_0805_2012Metric",
    ("unit 1 tms.kicad_sch", "R2"): "Resistor_SMD:R_0805_2012Metric",
    ("unit 1 tms.kicad_sch", "R3"): "Resistor_SMD:R_0805_2012Metric",
    ("unit 1 tms.kicad_sch", "R4"): "Resistor_SMD:R_0805_2012Metric",
    ("unit 1 tms.kicad_sch", "C1"): "Capacitor_SMD:C_0805_2012Metric",
    ("unit 1 tms.kicad_sch", "C2"): "Capacitor_SMD:C_0805_2012Metric",
    ("unit 1 tms.kicad_sch", "C3"): "Capacitor_SMD:C_0805_2012Metric",
    ("unit 1 tms.kicad_sch", "C4"): "Capacitor_SMD:C_0805_2012Metric",
    ("unit 1 tms.kicad_sch", "C_DB1"): "Capacitor_SMD:C_0805_2012Metric",
    ("unit 1 tms.kicad_sch", "SW1"): "Button_Switch_SMD:SW_SPST_PTS645Sx43SMTR92",

    # Unit 2
    ("unit 2 tms.kicad_sch", "U1"): "RF_Module:ESP32-WROOM-32",
    ("unit 2 tms.kicad_sch", "U2"): "Package_TO_SOT_SMD:SOT-223-3_TabPin2",
    ("unit 2 tms.kicad_sch", "J1"): "Connector_USB:USB_C_Receptacle_GCT_USB4125-xx-x_6P_TopMnt_Horizontal",
    ("unit 2 tms.kicad_sch", "R1"): "Resistor_SMD:R_0805_2012Metric",
    ("unit 2 tms.kicad_sch", "R2"): "Resistor_SMD:R_0805_2012Metric",
    ("unit 2 tms.kicad_sch", "R3"): "Resistor_SMD:R_0805_2012Metric",
    ("unit 2 tms.kicad_sch", "R4"): "Resistor_SMD:R_0805_2012Metric",
    ("unit 2 tms.kicad_sch", "C1"): "Capacitor_SMD:C_0805_2012Metric",
    ("unit 2 tms.kicad_sch", "C2"): "Capacitor_SMD:C_0805_2012Metric",
    ("unit 2 tms.kicad_sch", "C3"): "Capacitor_SMD:C_0805_2012Metric",
    ("unit 2 tms.kicad_sch", "C4"): "Capacitor_SMD:C_0805_2012Metric",
    ("unit 2 tms.kicad_sch", "C_DB1"): "Capacitor_SMD:C_0805_2012Metric",
    ("unit 2 tms.kicad_sch", "C_DB2"): "Capacitor_SMD:C_0805_2012Metric",
    ("unit 2 tms.kicad_sch", "C_DB3"): "Capacitor_SMD:C_0805_2012Metric",
    ("unit 2 tms.kicad_sch", "SW1"): "Button_Switch_SMD:SW_SPST_PTS645Sx43SMTR92",
    ("unit 2 tms.kicad_sch", "SW2"): "Button_Switch_SMD:SW_SPST_PTS645Sx43SMTR92",
    ("unit 2 tms.kicad_sch", "SW3"): "Button_Switch_SMD:SW_SPST_PTS645Sx43SMTR92",
}

LIB_DEFAULTS = {
    "Connector:USB_C_Receptacle_PowerOnly_6P": "Connector_USB:USB_C_Receptacle_GCT_USB4125-xx-x_6P_TopMnt_Horizontal",
    "Regulator_Linear:AMS1117-3.3": "Package_TO_SOT_SMD:SOT-223-3_TabPin2",
    "Device:R": "Resistor_SMD:R_0805_2012Metric",
    "Device:C": "Capacitor_SMD:C_0805_2012Metric",
    "Switch:SW_Push": "Button_Switch_SMD:SW_SPST_PTS645Sx43SMTR92",
    "RF_Module:ESP32-WROOM-32": "RF_Module:ESP32-WROOM-32"
}

def update_schematic(fname):
    filepath = os.path.join("schematics", fname)
    backup_path = filepath + ".bak"
    shutil.copyfile(filepath, backup_path)
    print(f"Backed up {fname} to {backup_path}")

    with open(filepath, "r", encoding="utf-8") as f:
        content = f.read()

    # 1. Update lib_symbols defaults
    for lib_id, def_fp in LIB_DEFAULTS.items():
        pattern = rf'(\(symbol\s+"{re.escape(lib_id)}"[\s\S]*?\(property\s+"Footprint"\s+)"[^"]*"'
        def repl_lib(m):
            return m.group(1) + f'"{def_fp}"'
        content, n = re.subn(pattern, repl_lib, content, count=1)
        if n > 0:
            print(f"[{fname}] Updated default footprint for lib_symbol {lib_id} -> {def_fp}")

    # 2. Update symbol instances
    # Each symbol instance in schematic:
    # \t(symbol\n\t\t(lib_id "...") ... \t\t(property "Reference" "REF" ...) ... \t\t(property "Footprint" "..." ...)
    # Let's split content into symbol instances carefully
    parts = re.split(r'(\n\t\(symbol\n)', content)
    # parts[0] is preamble before first instance symbol
    # parts[1] is '\n\t(symbol\n'
    # parts[2] is instance body
    # and so on

    new_parts = [parts[0]]
    for i in range(1, len(parts), 2):
        sym_header = parts[i]
        sym_body = parts[i+1]

        # Check Reference
        ref_m = re.search(r'\(property\s+"Reference"\s+"([^"]*)"', sym_body)
        if ref_m:
            ref = ref_m.group(1)
            key = (fname, ref)
            if key in ASSIGNMENTS:
                assigned_fp = ASSIGNMENTS[key]
                # Replace the Footprint property inside this sym_body
                old_fp_m = re.search(r'(\(property\s+"Footprint"\s+)"[^"]*"', sym_body)
                if old_fp_m:
                    old_fp = old_fp_m.group(0)
                    new_fp = f'{old_fp_m.group(1)}"{assigned_fp}"'
                    sym_body = sym_body.replace(old_fp, new_fp, 1)
                    print(f"[{fname}] Set {ref} Footprint -> {assigned_fp}")
                else:
                    print(f"[{fname}] Warning: Footprint property not found in {ref}")
        new_parts.append(sym_header)
        new_parts.append(sym_body)

    updated_content = "".join(new_parts)

    with open(filepath, "w", encoding="utf-8") as f:
        f.write(updated_content)
    print(f"Successfully saved {filepath}\n")

update_schematic("unit 1 tms.kicad_sch")
update_schematic("unit 2 tms.kicad_sch")
