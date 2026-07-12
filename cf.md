macro type shi

what a macro would have?
- keypresses
- delay


macAct: [keycode][act][delay]
- keycode: keycode (obiviously)
- act: action, like hold, release or press
- delay: do i have to explain ts?


macro: [actCount][[a1][a2][a3][a4]...[a24]] -> maximum 24 actions

needed:
- overflow handler
- meneys

menu structure type shi
```text
main
- Slot 1
  + Edit Macro
    * Text
      > Edit
        # Text: {smth}
        # Delay
      > Delete
      > Insert Action
    * Key
      > Edit
        # Keycode: {smth}
        # Modifier: {smth but pro max}
        # Delay
      // jdisoqjdsiojdo
    * Delay
      > Edit
        # Delay: {xejioqw}
    * [Add]
      > Key Press
        # // shi same as above
      > String
      > Delay
  + Run Macro
  + Save Macro
  + Load Macro
- Slot 2
- Slot 3
```