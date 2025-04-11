context = {
	[0] = 0,
	x = 0,
	y = 0,
	size = 10,
	vx = 10,
	vy = 1,
	entities = {
		player1 = {
			x = 0, y = 0
		},
		player2 = {
			x = 1, y = 0
		},
	},
}

function init()
	print("Hello World\n")
	context.font = load_font(".ttf", 30)
end

function cleanup()
	unload_font(context.font)
end

function draw()
	start = os.clock()
	if context.x < 0 then
		context.x = 0
		context.vx = -context.vx
	end
	if context.y < 0 then
		context.y = 0
		context.vy = -context.vy
	end
	max_width = 600
	if context.x > max_width - context.size then
		context.x = max_width - context.size
		context.vx = -context.vx
	end
	if context.y > 500 - context.size then
		context.y = 500 - context.size
		context.vy = -context.vy
	end

	if context.x + context.size <= (-context.x+max_width) and context.vx < 0 then
		context.vx = -context.vx
	end

	context.x = context.x + context.vx
	context.y = context.y + context.vy

	draw_rect(context.x, context.y, context.size*10, context.size, 0xfff0007f)
	draw_circle(-context.x+max_width, context.y, context.size, 0x00f0f07f)
	draw_text(context.font, string.format("Last frame took: %.7fs", os.clock() - start), 0, 0, 30, 1, 0xffffffff)

	draw_text(context.font, os.date("%d/%m/%Y %X"), 0, 500-30, 30, 1, 0xff0000ff)
end

function printtable(tb, level)
	if type(tb) ~= "table" then
		print(tb)
		return
	end
	for key, val in pairs(tb) do
		print(string.rep(" ", level*4) .. tostring(key), value, type(val))
		if type(val) == "table" then
			printtable(val, level + 1)
		end
	end
end

function getcontext()
	return context
end

function setcontext(ncontext)
	context = ncontext
end
