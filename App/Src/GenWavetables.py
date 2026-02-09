import numpy as np

TABLE_SIZE = 4096
FILE_NAME = "waveforms.cpp"

def get_t():
    return np.linspace(0, 1, TABLE_SIZE, endpoint=False)

def normalize(data):
    return data / np.max(np.abs(data))

def generate_waveforms():
    t = get_t()
    waves = {}

    # 1-3: The Foundations
    waves["Sine"] = np.sin(2 * np.pi * t)
    waves["Saw"] = normalize(sum((1.0/n) * np.sin(2 * np.pi * n * t) for n in range(1, 30)))
    waves["Square"] = normalize(sum((1.0/n) * np.sin(2 * np.pi * n * t) for n in range(1, 40, 2)))

    # 4. Rhodes: Fundamental + 2nd Harmonic + High Tine Shimmer
    rhodes = (1.0 * np.sin(2 * np.pi * t) + 
              0.4 * np.sin(4 * np.pi * t) + 
              0.2 * np.sin(2 * np.pi * 8.2 * t))
    waves["Rhodes"] = normalize(rhodes)

    # 5. Clav (10% Pulse): Very thin for percussive, funky keyboard sounds
    waves["Clav"] = normalize(sum((np.sin(n * np.pi * 0.10) / n) * np.cos(2 * np.pi * n * t) for n in range(1, 30)))

    # 6. Choir (Formant): Harmonic peaks at 1kHz and 3kHz (relative to a 100Hz base)
    # This approximates a "Vocal" shape
    choir = sum(np.exp(-((n-3)**2)/2) * np.sin(2 * np.pi * n * t) for n in range(1, 15))
    waves["Choir"] = normalize(choir)

    # 7. Acid: Asymmetric "Squelch" Wave
    t = get_t()
    ramp = (1.0 - t) 
    spike = np.exp(-t * 8.0) * np.sin(np.pi * t)
    acid_v2 = normalize(0.7 * ramp + 0.5 * spike)
    
    waves["Acid"] = acid_v2

    # 8. Glass (Bells): Inharmonic partials (non-integer multiples)
    # Using 1.0, 2.76, 5.4, and 8.1 for a metallic, glassy tone
    glass = (np.sin(2 * np.pi * t) + 
             0.5 * np.sin(2 * np.pi * 2.76 * t) + 
             0.3 * np.sin(2 * np.pi * 5.4 * t) + 
             0.2 * np.sin(2 * np.pi * 8.1 * t))
    waves["Glass"] = normalize(glass)

    write_to_file(waves)

def write_to_file(waves):
    with open(FILE_NAME, "w") as f:
        f.write('#include "waveforms.h"\n\n')
        for name, data in waves.items():
            f.write(f'alignas(4) const float waveform_{name}[{TABLE_SIZE}] = {{\n    ')
            formatted_data = [f"{v:.8f}f" for v in data]
            for i in range(0, len(formatted_data), 8):
                f.write(", ".join(formatted_data[i:i+8]) + ",\n    ")
            f.write("\n};\n\n")

if __name__ == "__main__":
    generate_waveforms()
