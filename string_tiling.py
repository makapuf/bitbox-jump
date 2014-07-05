import sys

# regarde si on peut rajouter au debut/ a la fin ou si dedans seulement
EXTRA_CHECK = True

def n_common(s1,s2,align) :
	n = min(len(s1),len(s2))
	for i in range(n-n%align,0,-align) :
		if s1[-i:]==s2[:i] :
			return i
	return 0

def tile(ll, align=1) :
	'''tile strings (of bytes), with the most dense packing possible
	input : list of strings
	ouput : string + list of (start,len)
	'''
	data= []
	idx = [] # string, ptr in string

	stat_data = {} # action : {n:number}
	gain=0

	for i,s in enumerate(ll) :
		max_j =None; max_n=0;where="nowhere"
		for j in range(0,len(data),align) :
			# regarde si deja connue
			if s in data[j] and data[j].index(s)%align == 0:
				max_n=len(s)
				max_j=j
				where='inside'

			# regarde si peut etre mis dedans
			if data[j] in s and s.index(data[j])%align==0:
				max_n=len(s)
				max_j=j
				where='outside'

			if EXTRA_CHECK :
				# regarde a la fin
				n=n_common(data[j],s,align)
				if n>max_n :
					max_n=n
					max_j=j
					where='end'

				# regarde au debut
				n=n_common(s,data[j],align)
				if n>max_n :
					max_n=n
					max_j=j
					where='start'
		#print s,where,max_j,max_n,data[max_j] if type(max_j)==int else "new"
		d=stat_data.setdefault(where,{})
		d[max_n] = d.get(max_n,0)+1
		gain += max_n

		if where=="end" :
			k=len(data[max_j])-max_n
			data[max_j]=data[max_j]+s[max_n:]
			idx.append((max_j,k,len(s)))

		elif where=='start' :
			data[max_j] = s[:-max_n]+data[max_j]
			# rewrite all matching indices
			for i,x in enumerate(idx) :
				if x[0]==max_j :
					idx[i]=(x[0],x[1]+len(s)-max_n,x[2])
			idx.append((max_j,0,len(s)))

		elif where=="inside" :
			idx.append((max_j,data[max_j].index(s),len(s)))

		elif where=="outside" :
			# rewrite all matching indices
			for i,x in enumerate(idx) :
				if x[0]==max_j :
					idx[i]=(x[0],x[1]+s.index(data[max_j]),x[2])
			data[max_j]=s
			idx.append((max_j,0,len(s)))

		elif where=="nowhere" :
			data.append(s)
			idx.append((len(data)-1,0,len(s)))

	# agregate all now
	if type(data[0])==str : 
		data_aggregate=''.join(data)
	else :  # generic list
		data_aggregate=sum(data,[])

	start_idx=[0] # starting index if data elements
	for d in data :	start_idx.append(start_idx[-1]+len(d))

	idx2=[] # new indices in aggregated data
	for x in idx :
		idx2.append((start_idx[x[0]]+x[1],x[2]))

	'''
	print >>sys.stderr,"tiling stats (align=%d) out=%d, %d refs"%(align,len(data_aggregate),len(idx2))
	for v,d in sorted(stat_data.items()) :
		print >>sys.stderr,'    ',v,'avg:',1.*sum(a*b for a,b in d.items())/sum(d.values()),'n:',sum(d.values()),'sum:',sum(a*b for a,b in d.items())
	print >>sys.stderr,'total gained:',gain
	'''

	return data_aggregate,idx2

def fake_tile(ll,align=1) :
	'just append them'
	data=[]
	idx=[]
	for i,s in enumerate(ll) :
		data.append(s)
		idx.append((len(data)-1,0,len(s)))
	# agregate all now
	data_aggregate=''.join(data)

	start_idx=[0] # starting index if data elements
	for d in data :	start_idx.append(start_idx[-1]+len(d))

	idx2=[] # new indices in aggregated data
	for x in idx :
		idx2.append((start_idx[x[0]]+x[1],x[2]))


	return data_aggregate,idx2


def test_tiling() :
	test_tuples = [
	('bbcfa','cfaxbbb',3),
	('aaaabbb','aaa',0),
	('aaa','aaabbb',3),
	('','aaacde',0),
	('aaacde','aaacde',6),
	('aaabbba','bbbaxxxx',4),
	]

	for s1,s2,n in test_tuples :
		if n_common(s1,s2,1) != n :
			raise ValueError,"Error, nocommon(%s,%s)==%d should be %d"%(s1,s2,n_common(s1,s2,1),n)


	s='xxbb','mxxbb',"aaaabbbbdddd","ddddaaaaxxbbbb",'','aaxx','ccccddddx','bbbbdddd','zccccdddd','ccccd','bbbd'
	# check decoding possible !

	data,idx = tile(s,4)
	print 'data',data
	print 'indexes',idx

	for ss,(a,b) in zip(s,idx) :
		assert ss==data[a:a+b] # Verify

	print sum(len(x) for x in s),'-->', len(data),',',len(idx)*2

	print tile([[1,2,3],[2,3,4]])

if __name__=='__main__' :

	test_tiling()

