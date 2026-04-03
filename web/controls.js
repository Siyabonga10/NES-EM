/**
 * typedef struct
{
    bool a_pressed;
    bool b_pressed;
    bool up_pressed;
    bool down_pressed;
    bool left_pressed;
    bool right_pressed;
    bool start_pressed;
    bool select_pressed;
} ControllerKeyStates;

The corresponding C code we interface with

 * 
 * 
 */


const updateKeyState = (index, setToTrue) => {
  if (keyStatesPtr === undefined) return;
  nesModule.HEAPU8[keyStatesPtr + index] = setToTrue ? 1 : 0;
}

document.addEventListener('keydown', (event) => {
  if (event.key.toLowerCase() === 'a') updateKeyState(0, true);
  if (event.key.toLowerCase() === 'b') updateKeyState(1, true);
  if (event.key.toLowerCase() === 'i') updateKeyState(2, true);
  if (event.key.toLowerCase() === 'k') updateKeyState(3, true);
  if (event.key.toLowerCase() === 'j') updateKeyState(4, true);
  if (event.key.toLowerCase() === 'l') updateKeyState(5, true);
  if (event.key.toLowerCase() === ' ') updateKeyState(6, true);
  if (event.key.toLowerCase() === 'enter') updateKeyState(7, true);
});

document.addEventListener('keyup', (event) => {
  if (event.key.toLowerCase() === 'a') updateKeyState(0, false);
  if (event.key.toLowerCase() === 'b') updateKeyState(1, false);
  if (event.key.toLowerCase() === 'i') updateKeyState(2, false);
  if (event.key.toLowerCase() === 'k') updateKeyState(3, false);
  if (event.key.toLowerCase() === 'j') updateKeyState(4, false);
  if (event.key.toLowerCase() === 'l') updateKeyState(5, false);
  if (event.key.toLowerCase() === ' ') updateKeyState(6, false);
  if (event.key.toLowerCase() === 'enter') updateKeyState(7, false);
});

