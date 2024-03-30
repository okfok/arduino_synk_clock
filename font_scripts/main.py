import input


def divide_chunks(l, n):
    # looping till length l
    for i in range(0, len(l), n):
        yield l[i:i + n]


class Digit:
    def __init__(self, data: list[list[int]]):
        self.data = data

    @classmethod
    def from_list(cls, x: int, y: int, lst: list[int]):
        assert x * y == len(lst)

        digit_data = []

        for line in divide_chunks(lst, x):
            digit_data.append(line)

        return cls(digit_data)

    @property
    def str(self):
        return ''.join([' '.join([str(j) + ',' for j in i]) + '\n' for i in self.data])

    def __mul__(self, other: int):
        x = len(self.data[0])
        y = len(self.data)
        data = [[0 for _ in range(x * other)] for _ in range(y * other)]

        for row in range(y):
            for col in range(x):
                for row_add in range(other):
                    for col_add in range(other):
                        data[row * other + row_add][col * other + col_add] = self.data[row][col]

        return Digit(data)


class Font:

    def __init__(self, x, y, digits: list[Digit]):
        self.x = x
        self.y = y
        self.digits = digits

    @classmethod
    def from_list(cls, x, y, count, lst: list[int]):
        assert x * y * count == len(lst)

        digits = []

        for digit_data in divide_chunks(lst, x * y):
            digits.append(Digit.from_list(x, y, digit_data))

        return cls(x, y, digits)

    @property
    def str(self):
        return '{' + '\n'.join(map(lambda x: x.str, self.digits)) + '}'

    def __mul__(self, other: int):
        return Font(self.x * other, self.y * other, [digit * other for digit in self.digits])


font = Font.from_list(8, 16, 256, input.data)
font.digits = font.digits[:128]
# print(font.digits[1].str, '\n\n\n', (font.digits[1] * 2).str)

with open('CodePage437_40x80.h', 'w') as f:
    f.write((font * 5).str)
