import chess, random
from tqdm import tqdm

def random_fen():
	board = chess.Board(fen="r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1")

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
	num_fens = 100_000
	fens, moves = zip(*[random_fen() for _ in tqdm(range(num_fens))])

	with open('fens.txt', 'w') as f:
		f.write('\n'.join(fens))
	
	with open('moves.txt', 'w') as f:
		f.write('\n'.join(moves))

main()