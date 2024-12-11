import mido

def midi_note_to_frequency(note):
    """Convert MIDI note number to frequency in Hz."""
    return 440.0 * (2.0 ** ((note - 69) / 12.0))

def midi_to_frequency_duration_pairs(midi_file):
    """Convert MIDI file to frequency-duration pairs (directly using time)."""
    mid = mido.MidiFile(midi_file)
    pairs = []
    active_notes = {}

    for msg in mid:
        if msg.type == 'note_on' and msg.velocity > 0:
            # Note ON: Start tracking
            active_notes[msg.note] = msg.time
        elif msg.type == 'note_off' or (msg.type == 'note_on' and msg.velocity == 0):
            # Note OFF: Calculate duration
            if msg.note in active_notes:
                duration_seconds = msg.time  # Directly use the time
                frequency = midi_note_to_frequency(msg.note)
                pairs.append((frequency, duration_seconds))
                del active_notes[msg.note]

    return pairs

# Usage
midi_file = 'single_voice.mid'
pairs = midi_to_frequency_duration_pairs(midi_file)

# Print the frequency-duration pairs
for frequency, duration in pairs:
    print(f"{{{frequency:.2f}, {1000*duration:.3f}f}},")
