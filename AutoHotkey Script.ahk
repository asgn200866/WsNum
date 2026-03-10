#NoEnv
#SingleInstance, Force
CoordMode, Mouse, Screen  ; ВАЖНО: все координаты относительно всего виртуального стола

; ========== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ==========

; Функция для безопасного перемещения мыши с проверкой
SafeClick(X, Y) {
    ; Проверяем, что координаты в разумных пределах
    if (Abs(X) > 5000 or Abs(Y) > 5000) {
        ToolTip, Ошибка: координаты X=%X% Y=%Y% вне диапазона!
        Sleep, 1000
        ToolTip
        return false
    }
    
    ; Перемещаем и кликаем
    MouseMove, %X%, %Y%, 0
    Sleep, 30  ; маленькая пауза для стабильности
    Click
    return true
}

; Функция для возврата мыши с защитой от "вылета за экран"
SafeReturn(OriginalX, OriginalY) {
    ; Проверяем, что исходные координаты валидны
    if (OriginalX != "" and OriginalY != "" and Abs(OriginalX) < 5000 and Abs(OriginalY) < 5000) {
        MouseMove, %OriginalX%, %OriginalY%, 0
    } else {
        ; Если что-то пошло не так - перемещаем в центр основного монитора
        MouseMove, 0, 0, 0  ; левый верхний угол основного
    }
}

; ========== ТВОИ КОМБИНАЦИИ (АДАПТИРОВАНЫ) ==========

; Двойной Ctrl - клик и возврат
~Ctrl::
    if (A_PriorHotkey <> "~Ctrl" or A_TimeSincePriorHotkey > 300)
    {
        KeyWait, Ctrl
        return
    }
    
    ; Запоминаем где были
    MouseGetPos, OriginalX, OriginalY
    
    ; Выполняем действия
    SafeClick(2100, 0)
    SafeClick(2980, -430)
    
    ; Возвращаем мышь обратно
    SafeReturn(OriginalX, OriginalY)
return

; Двойной Alt - два клика подряд
~Alt::
    if (A_PriorHotkey <> "~Alt" or A_TimeSincePriorHotkey > 300)
    {
        KeyWait, Alt
        return
    }
    
    MouseGetPos, OriginalX, OriginalY
    
    SafeClick(2315, 475)
    SafeClick(2660, 660)
    
    SafeReturn(OriginalX, OriginalY)
return

; Двойной Enter - клик + вставка + Enter
~Enter::
    ; ВАЖНО: для Enter в A_PriorHotkey может быть пробел!
    if (A_PriorHotkey <> "~Enter" and A_PriorHotkey <> "~Enter " or A_TimeSincePriorHotkey > 300)
    {
        KeyWait, Enter
        return
    }
    
    MouseGetPos, OriginalX, OriginalY
    
    ; Клик
    SafeClick(2310, 660)
    ; Вставка и Enter
    Send, ^v
    Send, {Enter}
    
    SafeReturn(OriginalX, OriginalY)
return

; Двойной Right  - клик
~Right:: 
    if (A_PriorHotkey <> "~Right" or A_TimeSincePriorHotkey > 300)
    {
        KeyWait, Right
        return
    }
    
    MouseGetPos, OriginalX, OriginalY
    
    SafeClick(2100, 960)
    
    SafeReturn(OriginalX, OriginalY)
return

; Двойной Down   - клик
~Down:: 
    if (A_PriorHotkey <> "~Down" or A_TimeSincePriorHotkey > 300)
    {
        KeyWait, Down
        return
    }
    
    MouseGetPos, OriginalX, OriginalY
    
    SafeClick(2660, 620)
    
    SafeReturn(OriginalX, OriginalY)
return

; ========== ДОПОЛНИТЕЛЬНЫЕ ПОЛЕЗНЫЕ ШТУКИ ==========

; Alt+D - показать текущие координаты (для отладки)
!d::
    MouseGetPos, X, Y
    CoordMode, Mouse, Screen
    ToolTip, X=%X% Y=%Y%, X+20, Y+20
    Sleep, 2000
    ToolTip
return

; Alt+M - показать границы мониторов
!m::
    SysGet, MonitorCount, 80
    Result := "Мониторов: " MonitorCount "`n`n"
    
    Loop, %MonitorCount%
    {
        SysGet, Mon, Monitor, %A_Index%
        Result .= "Монитор " A_Index ":"
        Result .= "  Левый: " MonLeft
        Result .= "  Правый: " MonRight
        Result .= "  Верх: " MonTop
        Result .= "  Низ: " MonBottom
        Result .= "`n"
    }
    
    MouseGetPos, MX, MY
    Result .= "`nСейчас мышь: X=" MX " Y=" MY
    
    MsgBox, %Result%
return