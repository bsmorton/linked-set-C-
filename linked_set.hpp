// Submitter: bsmorton(Morton, Bradley)
#ifndef LINKED_SET_HPP_
#define LINKED_SET_HPP_

#include <string>
#include <iostream>
#include <sstream>
#include <initializer_list>
#include "ics_exceptions.hpp"


namespace ics {


template<class T> class LinkedSet {
  public:
    //Destructor/Constructors
    ~LinkedSet();

    LinkedSet          ();
    explicit LinkedSet (int initialLength);
    LinkedSet          (const LinkedSet<T>& to_copy);
    explicit LinkedSet (const std::initializer_list<T>& il);

    //Iterable class must support "for-each" loop: .begin()/.end() and prefix ++ on returned result
    template <class Iterable>
    explicit LinkedSet (const Iterable& i);


    //Queries
    bool empty      () const;
    int  size       () const;
    bool contains   (const T& element) const;
    std::string str () const; //supplies useful debugging information; contrast to operator <<

    //Iterable class must support "for-each" loop: .begin()/.end() and prefix ++ on returned result
    template <class Iterable>
    bool contains_all (const Iterable& i) const;


    //Commands
    int  insert (const T& element);
    int  erase  (const T& element);
    void clear  ();

    //Iterable class must support "for" loop: .begin()/.end() and prefix ++ on returned result

    template <class Iterable>
    int insert_all(const Iterable& i);

    template <class Iterable>
    int erase_all(const Iterable& i);

    template<class Iterable>
    int retain_all(const Iterable& i);


    //Operators
    LinkedSet<T>& operator = (const LinkedSet<T>& rhs);
    bool operator == (const LinkedSet<T>& rhs) const;
    bool operator != (const LinkedSet<T>& rhs) const;
    bool operator <= (const LinkedSet<T>& rhs) const;
    bool operator <  (const LinkedSet<T>& rhs) const;
    bool operator >= (const LinkedSet<T>& rhs) const;
    bool operator >  (const LinkedSet<T>& rhs) const;

    template<class T2>
    friend std::ostream& operator << (std::ostream& outs, const LinkedSet<T2>& s);



  private:
    class LN;

  public:
    class Iterator {
      public:
        //Private constructor called in begin/end, which are friends of LinkedSet<T>
        ~Iterator();
        T           erase();
        std::string str  () const;
        LinkedSet<T>::Iterator& operator ++ ();
        LinkedSet<T>::Iterator  operator ++ (int);
        bool operator == (const LinkedSet<T>::Iterator& rhs) const;
        bool operator != (const LinkedSet<T>::Iterator& rhs) const;
        T& operator *  () const;
        T* operator -> () const;
        friend std::ostream& operator << (std::ostream& outs, const LinkedSet<T>::Iterator& i) {
          outs << i.str(); //Use the same meaning as the debugging .str() method
          return outs;
        }
        friend Iterator LinkedSet<T>::begin () const;
        friend Iterator LinkedSet<T>::end   () const;

      private:
        //If can_erase is false, current indexes the "next" value (must ++ to reach it)
        LN*           current;  //if can_erase is false, this value is unusable
        LinkedSet<T>* ref_set;
        int           expected_mod_count;
        bool          can_erase = true;

        //Called in friends begin/end
        Iterator(LinkedSet<T>* iterate_over, LN* initial);
    };


    Iterator begin () const;
    Iterator end   () const;


  private:
    class LN {
      public:
        LN ()                      {}
        LN (const LN& ln)          : value(ln.value), next(ln.next){}
        LN (T v,  LN* n = nullptr) : value(v), next(n){}

        T   value;
        LN* next   = nullptr;
    };


    LN* front     = new LN();
    LN* trailer   = front;         //Always point to special trailer LN
    int used      =  0;            //Cache the number of values in linked list
    int mod_count = 0;             //For sensing concurrent modification

    //Helper methods
    int  erase_at   (LN* p);
    void delete_list(LN*& front);  //Deallocate all LNs (but trailer), and set front's argument to trailer;
};





////////////////////////////////////////////////////////////////////////////////
//
//LinkedSet class and related definitions

//Destructor/Constructors

template<class T>
LinkedSet<T>::~LinkedSet() {
    LN* current = trailer;
    while( current != nullptr ) {
        LN* next = current->next;
        delete current;
        current = next;
    }
    trailer = nullptr;
}


template<class T>
LinkedSet<T>::LinkedSet() {
}


template<class T>
LinkedSet<T>::LinkedSet(const LinkedSet<T>& to_copy) : used(to_copy.used) {
    used=0;
    insert_all(to_copy);
}


template<class T>
LinkedSet<T>::LinkedSet(const std::initializer_list<T>& il) {
   insert_all(il);
}


template<class T>
template<class Iterable>
LinkedSet<T>::LinkedSet(const Iterable& i) {
    insert_all(i);
}


////////////////////////////////////////////////////////////////////////////////
//
//Queries

template<class T>
bool LinkedSet<T>::empty() const {
    return used==0;
}


template<class T>
int LinkedSet<T>::size() const {
    return used;
}


template<class T>
bool LinkedSet<T>::contains (const T& element) const {
    for (LN *p = trailer->next; p != nullptr; p = p->next){
        if(p->value==element){
            return true;
        }
    }
    return false;
}


template<class T>
std::string LinkedSet<T>::str() const {
    std::ostringstream answer;
    answer << "LinkedSet[";
    if (used != 0) {
        int i=0;
        for (LN *p = trailer->next; p != nullptr; p = p->next){
            if(p->next!= nullptr){
                answer << i <<":" << p->value << ",";
            }
            else{
                answer << i << ":" << p->value;
            }
            i++;
        }
    }

    answer << "](used=" << used << ",mod_count=" << mod_count << ")";
    return answer.str();
}


template<class T>
template<class Iterable>
bool LinkedSet<T>::contains_all (const Iterable& i) const {
    for(T& p:i) {
        if (!contains(p))
            return false;
    }
    return true;
}


////////////////////////////////////////////////////////////////////////////////
//
//Commands


template<class T>
int LinkedSet<T>::insert(const T& element) {
    if(contains(element)){
        return 0;
    }
    front->next=new LN(element);
    front=front->next;
    used+=1;
    return 1;
}


template<class T>
int LinkedSet<T>::erase(const T& element) {
    for (LN *p = trailer; p->next != nullptr; p = p->next){
        if(p->next->value==element){
            LN* to_delete = p->next;
            p->next=p->next->next;
            delete to_delete;
            used-=1;

            return 1;
        }
    }
    return 0;

}


template<class T>
void LinkedSet<T>::clear() {
    front = new LN();
    trailer  = front;
    used = 0;

}


template<class T>
template<class Iterable>
int LinkedSet<T>::insert_all(const Iterable& i) {
    int count=0;
    for(const T& p:i){
        count+=insert(p);
    }
    return count;
}


template<class T>
template<class Iterable>
int LinkedSet<T>::erase_all(const Iterable& i) {
    int erased=0;
    for(T& p:i){
        erased+=1;
        erase(p);
    }
    return erased;
}


template<class T>
template<class Iterable>
int LinkedSet<T>::retain_all(const Iterable& i) {

}


////////////////////////////////////////////////////////////////////////////////
//
//Operators

template<class T>
LinkedSet<T>& LinkedSet<T>::operator = (const LinkedSet<T>& rhs) {
    if (this == &rhs){
        return *this;
    }
    clear();
    for (LN *p = rhs.trailer->next; p != nullptr; p = p->next){
        insert(p->value);
    }
    return *this;
}


template<class T>
bool LinkedSet<T>::operator == (const LinkedSet<T>& rhs) const {
    if(size()!=rhs.size()){
        return false;
    }
    for (LN *p = trailer->next; p != nullptr; p = p->next){
        if(!rhs.contains(p->value)){
            return false;
        }
    }
    return true;
}


template<class T>
bool LinkedSet<T>::operator != (const LinkedSet<T>& rhs) const {
    return !(*this==rhs);
}


template<class T>
bool LinkedSet<T>::operator <= (const LinkedSet<T>& rhs) const {
    if(rhs.size()<size()){
        return false;
    }
    for (LN *p = trailer->next; p != nullptr; p = p->next){
        if(!rhs.contains(p->value)){
            return false;
        }
    }
    return true;
}


template<class T>
bool LinkedSet<T>::operator < (const LinkedSet<T>& rhs) const {
    return !(*this>=rhs);
}


template<class T>
bool LinkedSet<T>::operator >= (const LinkedSet<T>& rhs) const {
    if(rhs.size()>size()){
        return false;
    }
    for (LN *p = rhs.trailer->next; p != nullptr; p = p->next){
        if(!contains(p->value)){
            return false;
        }
    }
    return true;
}


template<class T>
bool LinkedSet<T>::operator > (const LinkedSet<T>& rhs) const {
    return !(*this<=rhs);
}


template<class T>
std::ostream& operator << (std::ostream& outs, const LinkedSet<T>& s) {
    outs << "set[";
    if(s.size()!=0){
        for (typename LinkedSet<T>::LN *p = s.trailer->next; p != nullptr; p = p->next){
            if(p->next!= nullptr){
                outs << p->value+",";
            }
            else{
                outs << p->value;
            }
        }
    }
    outs << "]";
    return outs;
}


////////////////////////////////////////////////////////////////////////////////
//
//Iterator constructors

template<class T>
auto LinkedSet<T>::begin () const -> LinkedSet<T>::Iterator {
    return Iterator(const_cast<LinkedSet<T>*>(this),trailer->next);
}


template<class T>
auto LinkedSet<T>::end () const -> LinkedSet<T>::Iterator {
    return Iterator(const_cast<LinkedSet<T>*>(this),front->next);
}


////////////////////////////////////////////////////////////////////////////////
//
//Private helper methods

template<class T>
int LinkedSet<T>::erase_at(LN* p) {
}


template<class T>
void LinkedSet<T>::delete_list(LN*& front) {
}





////////////////////////////////////////////////////////////////////////////////
//
//Iterator class definitions

template<class T>
LinkedSet<T>::Iterator::Iterator(LinkedSet<T>* iterate_over, LN* initial)
        : current(initial), ref_set(iterate_over), expected_mod_count(ref_set->mod_count)
{

}


template<class T>
LinkedSet<T>::Iterator::~Iterator()
{}


template<class T>
T LinkedSet<T>::Iterator::erase(){

}


template<class T>
std::string LinkedSet<T>::Iterator::str() const {
    std::ostringstream answer;
    answer << ref_set->str() << "(current=" << current << ",expected_mod_count=" << expected_mod_count << ",can_erase=" << can_erase << ")";
    return answer.str();
}


template<class T>
auto LinkedSet<T>::Iterator::operator ++ () -> LinkedSet<T>::Iterator& {
    if (current == ref_set->front->next){
        return *this;
    }
    current=current->next;
    return *this;
}


template<class T>
auto LinkedSet<T>::Iterator::operator ++ (int) -> LinkedSet<T>::Iterator {
    if (current == ref_set->front->next){
        return *this;
    }
    Iterator to_return(*this);
    current=current->next;
    return to_return;

}


template<class T>
bool LinkedSet<T>::Iterator::operator == (const LinkedSet<T>::Iterator& rhs) const {
    const Iterator* rhsASI = dynamic_cast<const Iterator*>(&rhs);
    if (rhsASI == 0)
        throw IteratorTypeError("ArrayQueue::Iterator::operator ==");
    if (ref_set != rhsASI->ref_set)
        throw ComparingDifferentIteratorsError("ArrayQueue::Iterator::operator !=");
    return current == rhsASI-> current;
}


template<class T>
bool LinkedSet<T>::Iterator::operator != (const LinkedSet<T>::Iterator& rhs) const {
    const Iterator* rhsASI = dynamic_cast<const Iterator*>(&rhs);
    if (rhsASI == 0)
        throw IteratorTypeError("ArrayQueue::Iterator::operator ==");
    if (ref_set != rhsASI->ref_set)
        throw ComparingDifferentIteratorsError("ArrayQueue::Iterator::operator !=");
    return current  != rhsASI-> current;
}


template<class T>
T& LinkedSet<T>::Iterator::operator *() const {

    return current->value;
}


template<class T>
T* LinkedSet<T>::Iterator::operator ->() const {
    return &current->value;
}


}

#endif /* LINKED_SET_HPP_ */
