import math

VREF = 3.3
ADC_MAX = 4095

R_FIXED = 10000.0   # bottom resistor to GND
R25 = 10000.0       # thermistor resistance at 25C
BETA = 3435.0       # Littelfuse datasheet beta
T25_K = 25.0 + 273.15


def thermistor_resistance_from_temp_c(temp_c: float) -> float:
    """
    Beta equation:
        R = R25 * exp(B * (1/T - 1/T25))
    T must be in Kelvin
    """
    temp_k = temp_c + 273.15
    return R25 * math.exp(BETA * ((1.0 / temp_k) - (1.0 / T25_K)))


def vout_from_resistance(r_ntc: float) -> float:
    """
    Divider:
        3.3V -> NTC -> ADC node -> 10k -> GND

        Vout = Vref * R_fixed / (R_ntc + R_fixed)
    """
    return VREF * (R_FIXED / (r_ntc + R_FIXED))


def adc_from_voltage(vout: float) -> int:
    return round((vout / VREF) * ADC_MAX)


def generate_lookup_table(temp_min: int = -30, temp_max: int = 90):
    table = []

    for temp_c in range(temp_max, temp_min - 1, -1):
        r_ntc = thermistor_resistance_from_temp_c(temp_c)
        vout = vout_from_resistance(r_ntc)
        adc = adc_from_voltage(vout)
        table.append((adc, temp_c, vout, r_ntc))

    return table


def print_c_table(table):
    print("static const ThermistorTableEntry_t s_temp_table[] = {")
    for i in range(0, len(table), 5):
        chunk = table[i:i+5]
        line = "    " + ", ".join(f"{{{adc:4d}, {temp:4d}}}" for adc, temp, _, _ in chunk)
        if i + 5 < len(table):
            line += ","
        print(line)
    print("};")


def print_debug_table(table):
    print(f"{'Temp(C)':>8} | {'ADC':>5} | {'Vout(V)':>8} | {'R_ntc(ohm)':>12}")
    print("-" * 44)
    for adc, temp, vout, r_ntc in table:
        print(f"{temp:8d} | {adc:5d} | {vout:8.4f} | {r_ntc:12.2f}")


if __name__ == "__main__":
    table = generate_lookup_table(-30, 90)

    print_c_table(table)

    