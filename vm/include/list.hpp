#ifndef LIST_HPP
#define LIST_HPP

template<class T>
	class List {
	  private:
	    int count;
	    int capacity;
	    T* values;

	  public:
	    List();
	    ~List();

	    void add(T value);
	    void free();

	    T get(int i) { return values[i]; }

	    int get_count() { return count; }
	    int get_capacity() { return capacity; }
};

#endif