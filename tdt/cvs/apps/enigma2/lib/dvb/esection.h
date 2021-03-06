#ifndef __esection_h
#define __esection_h

#include <lib/dvb/idemux.h>
#include <set>

#define TABLE_eDebug(x...) do { if (m_debug) eDebug(x); } while(0)
#define TABLE_eDebugNoNewLine(x...) do { if (m_debug) eDebugNoNewLine(x); } while(0)

class eGTable: public iObject, public Object
{
	DECLARE_REF(eGTable);
	ePtr<iDVBSectionReader> m_reader;
	eDVBTableSpec m_table;
	
	unsigned int m_tries;
	
	ePtr<eTimer> m_timeout;

	void sectionRead(const __u8 *data);
	void timeout();
	ePtr<eConnection> m_sectionRead_conn;
protected:
	bool m_debug;
	virtual int createTable(unsigned int nr, const __u8 *data, unsigned int max)=0;
public:
	Signal1<void, int> tableReady;
	eGTable(bool debug=true);
	RESULT start(iDVBSectionReader *reader, const eDVBTableSpec &table);
	RESULT start(iDVBDemux *reader, const eDVBTableSpec &table);
	RESULT getSpec(eDVBTableSpec &spec) { spec = m_table; return 0; }
	virtual ~eGTable();
	int error;
	int ready;
};

template <class Section>
class eTable: public eGTable
{
private:
	std::vector<Section*> sections;
	std::set<int> avail;
protected:
	int createTable(unsigned int nr, const __u8 *data, unsigned int max)
	{
		unsigned int ssize = sections.size();
		if (max < ssize || nr >= max)
		{
			TABLE_eDebug("kaputt max(%d) < ssize(%d) || nr(%d) >= max(%d)",
				max, ssize, nr, max);
			return 0;
		}
		if (avail.find(nr) != avail.end())
			delete sections[nr];

		sections.resize(max);
		sections[nr] = new Section(data);
		avail.insert(nr);

		for (unsigned int i = 0; i < max; ++i)
			if (avail.find(i) != avail.end())
				TABLE_eDebugNoNewLine("+");
			else
				TABLE_eDebugNoNewLine("-");
				
		TABLE_eDebug(" %d/%d TID %02x", avail.size(), max, data[0]);

		if (avail.size() == max)
		{
			TABLE_eDebug("done!");
			return 1;
		} else
			return 0;
	}
public:
	std::vector<Section*> &getSections() { return sections; }
	eTable(bool debug=true): eGTable(debug)
	{
	}
	~eTable()
	{
		for (std::set<int>::iterator i(avail.begin()); i != avail.end(); ++i)
			delete sections[*i];
	}
};

class eAUGTable: public Object
{
protected:
	void slotTableReady(int);
public:
	Signal1<void, int> tableReady;
	virtual void getNext(int err)=0;
};

template <class Table>
class eAUTable: public eAUGTable
{
	ePtr<Table> current, next;		// current is READY AND ERRORFREE, next is not yet ready
	int first;
	ePtr<iDVBDemux> m_demux;
	eMainloop *ml;
public:

	eAUTable()
	{
	}

	~eAUTable()
	{
		stop();
	}
	
	void stop()
	{
		current = next = 0;
		m_demux = 0;
	}
	
	int begin(eMainloop *m, const eDVBTableSpec &spec, ePtr<iDVBDemux> demux)
	{
		ml = m;
		m_demux = demux;
		first= 1;
		current = 0;
		next = new Table();
		CONNECT(next->tableReady, eAUTable::slotTableReady);
		next->start(demux, spec);
		return 0;
	}
	
	int get()
	{
		if (current)
		{
			/*emit*/ tableReady(0);
			return 0;
		} else if (!next)
		{
			/*emit*/ tableReady(-1);
			return 0;
		} else
			return 1;
	}
	
	RESULT getCurrent(ePtr<Table> &ptr)
	{
		if (!current)
			return -1;
		ptr = current;
		return 0;
	}

#if 0	
	void abort()
	{
		eDebug("eAUTable: aborted!");
		if (next)
			next->abort();
		delete next;
		next=0;
	}
#endif

	int ready()
	{
		return !!current;
	}
	
	void inject(Table *t)
	{
		next=t;
		getNext(0);
	}

	void getNext(int error)
	{
		current = 0;
		if (error)
		{
			next=0;
			if (first)
				/*emit*/ tableReady(error);
			first=0;
			return;
		} else
			current=next;

		next=0;
		first=0;
		
		ASSERT(current->ready);
			
		/*emit*/ tableReady(0);
		
		eDVBTableSpec spec;

		if (current && (!current->getSpec(spec)))
		{
			next = new Table();
			CONNECT(next->tableReady, eAUTable::slotTableReady);
			spec.flags &= ~(eDVBTableSpec::tfAnyVersion|eDVBTableSpec::tfThisVersion|eDVBTableSpec::tfHaveTimeout);
			next->eGTable::start(m_demux, spec);
		}
	}
};

#endif
