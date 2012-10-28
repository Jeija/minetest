print("---Client Lua API started---")

client.on_step = function (dtime)
	print("Hello World!".." | "..dtime .."\n")
end

function onstep ()
	print("Hello World\n")
end
