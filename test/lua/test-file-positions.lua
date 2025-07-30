-- Combined debug and production test to catch the issue
describe("File position conversion functions", function()
	local file = vis.win.file

	describe("file:linecol_by_pos()", function()
		it("should convert byte position to line and column", function()
			-- Position 0 should be line 1, column 1
			local l1, c1 = file:linecol_by_pos(0)
			if l1 ~= 1 or c1 ~= 1 then
				print(string.format("FAIL: linecol_by_pos(0) expected (1,1), got (%d,%d)", l1, c1))
			end
			assert.are.equal(1, l1)
			assert.are.equal(1, c1)

			-- Position 5 should be '1' in "Line 1" (line 1, column 6)
			local l2, c2 = file:linecol_by_pos(5)
			if l2 ~= 1 or c2 ~= 6 then
				print(string.format("FAIL: linecol_by_pos(5) expected (1,6), got (%d,%d)", l2, c2))
			end
			assert.are.equal(1, l2)
			assert.are.equal(6, c2)

			-- Position 7 should be start of line 2
			local l3, c3 = file:linecol_by_pos(7)
			if l3 ~= 2 or c3 ~= 1 then
				print(string.format("FAIL: linecol_by_pos(7) expected (2,1), got (%d,%d)", l3, c3))
			end
			assert.are.equal(2, l3)
			assert.are.equal(1, c3)

			-- Position 31 should be the empty line (line 3)
			local l4, c4 = file:linecol_by_pos(31)
			if l4 ~= 3 or c4 ~= 1 then
				print(string.format("FAIL: linecol_by_pos(31) expected (3,1), got (%d,%d)", l4, c4))
			end
			assert.are.equal(3, l4)
			assert.are.equal(1, c4)

			-- Position 35 should be 'n' in "Líne" (line 4, column 3, after 2-byte í)
			local l5, c5 = file:linecol_by_pos(35)
			if l5 ~= 4 or c5 ~= 3 then
				print(string.format("FAIL: linecol_by_pos(35) expected (4,3), got (%d,%d)", l5, c5))
			end
			assert.are.equal(4, l5)
			assert.are.equal(3, c5)
		end)
	end)

	describe("file:pos_by_linecol()", function()
		it("should convert line and column to byte position", function()
			-- Start of file should be position 0
			local pos1 = file:pos_by_linecol(1, 1)
			if pos1 ~= 0 then
				print(string.format("FAIL: pos_by_linecol(1,1) expected 0, got %s", pos1 or "nil"))
			end
			assert.are.equal(0, pos1)

			-- Start of line 2 should be position 7
			local pos2 = file:pos_by_linecol(2, 1)
			if pos2 ~= 7 then
				print(string.format("FAIL: pos_by_linecol(2,1) expected 7, got %s", pos2 or "nil"))
			end
			assert.are.equal(7, pos2)

			-- Column 6 of line 1 should be position 5 ('1' in "Line 1")
			local pos3 = file:pos_by_linecol(1, 6)
			if pos3 ~= 5 then
				print(string.format("FAIL: pos_by_linecol(1,6) expected 5, got %s", pos3 or "nil"))
			end
			assert.are.equal(5, pos3)

			-- Line 4, column 3 should be position 35 ('n' in "Líne")
			local pos4 = file:pos_by_linecol(4, 3)
			if pos4 ~= 35 then
				print(string.format("FAIL: pos_by_linecol(4,3) expected 35, got %s", pos4 or "nil"))
			end
			assert.are.equal(35, pos4)

			-- Invalid line should return nil
			local pos5 = file:pos_by_linecol(99)
			assert.are.equal(nil, pos5)
		end)
	end)

	describe("Symmetry", function()
		it("should be symmetrical when converting back and forth", function()
			-- Test with start of file
			local test_pos = 0
			local l, c = file:linecol_by_pos(test_pos)
			local pos_result = file:pos_by_linecol(l, c)
			if test_pos ~= pos_result then
				print(string.format("FAIL: symmetry test pos %d -> (%d,%d) -> %s", test_pos, l, c, pos_result or "nil"))
			end
			assert.are.equal(test_pos, pos_result)

			-- Test with position 5 ('1' in "Line 1")
			test_pos = 5
			l, c = file:linecol_by_pos(test_pos)
			pos_result = file:pos_by_linecol(l, c)
			if test_pos ~= pos_result then
				print(string.format("FAIL: symmetry test pos %d -> (%d,%d) -> %s", test_pos, l, c, pos_result or "nil"))
			end
			assert.are.equal(test_pos, pos_result)

			-- Test with UTF-8 position (position 35, 'n' in "Líne")
			test_pos = 35
			l, c = file:linecol_by_pos(test_pos)
			pos_result = file:pos_by_linecol(l, c)
			if test_pos ~= pos_result then
				print(string.format("FAIL: symmetry test pos %d -> (%d,%d) -> %s", test_pos, l, c, pos_result or "nil"))
			end
			assert.are.equal(test_pos, pos_result)

			-- Test line/col to pos to line/col
			local test_line, test_col = 2, 3
			local pos = file:pos_by_linecol(test_line, test_col)
			local l_result, c_result = file:linecol_by_pos(pos)
			if test_line ~= l_result or test_col ~= c_result then
				print(string.format("FAIL: symmetry test (%d,%d) -> %s -> (%d,%d)", test_line, test_col, pos or "nil", l_result, c_result))
			end
			assert.are.equal(test_line, l_result)
			assert.are.equal(test_col, c_result)

			-- Test with UTF-8 line/col
			test_line, test_col = 4, 3  -- 'n' in "Líne"
			pos = file:pos_by_linecol(test_line, test_col)
			l_result, c_result = file:linecol_by_pos(pos)
			if test_line ~= l_result or test_col ~= c_result then
				print(string.format("FAIL: symmetry test (%d,%d) -> %s -> (%d,%d)", test_line, test_col, pos or "nil", l_result, c_result))
			end
			assert.are.equal(test_line, l_result)
			assert.are.equal(test_col, c_result)
		end)
	end)
end)
