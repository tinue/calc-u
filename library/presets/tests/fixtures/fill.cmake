# The fill script of the library's preset check (calcu_add_preset_check FILL):
# @MODEL@ becomes test-seg, as a product fills its templates.
file(READ ${IN} text)
string(REPLACE "@MODEL@" "test-seg" text "${text}")
file(WRITE ${OUT} "${text}")
