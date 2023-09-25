import chess, random
from tqdm import tqdm

def random_fen():
	board = chess.Board()

	while True:
		try:
			moves = random.randint(0, 50)
			for _ in range(moves):
				move = random.choice(list(board.legal_moves))
				board.push(move)
		except:
			pass
		else:
			break
	
	legal_moves = [m.uci() for m in board.legal_moves]
	legal_moves.sort()

	return board.fen(), ' '.join(legal_moves)

def main():
	num_fens = 1_000_000
	fens, moves = zip(*[random_fen() for _ in tqdm(range(num_fens))])

	with open('fens.txt', 'w') as f:
		f.write('\n'.join(fens))
	
	with open('moves.txt', 'w') as f:
		f.write('\n'.join(moves))

main()