local text = pandoc.text

if FORMAT:match 'man' then
    function Header(el)
        local is_see_also = true
        local token = 0
        el:walk {
            Str = function(el)
                token = token + 1
                if (token == 1 and text.upper(el.text) ~= "SEE") or
                   (token == 2 and text.upper(el.text) ~= "ALSO") or
                   (token > 2) then
                    is_see_also = false
                end
            end
        }
        if is_see_also then
            return {
                el,
                pandoc.RawInline('man', '.ad l\n.nh\n')
            }
        else
            return el
        end
    end
end
