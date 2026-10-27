// Blocks browser DevTools access (right-click, F12, Ctrl+Shift+I/J/C, Ctrl+U)
document.addEventListener('contextmenu', e => e.preventDefault());

document.addEventListener('keydown', e => {
  if (
    e.key === 'F12' ||
    (e.ctrlKey && e.shiftKey && ['I', 'J', 'C'].includes(e.key.toUpperCase())) ||
    (e.ctrlKey && e.key.toUpperCase() === 'U')
  ) {
    e.preventDefault();
  }
});
