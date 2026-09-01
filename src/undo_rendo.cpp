#include <iostream>
#include <string>
#include <stack>
#include <memory>
#include <functional>

// Interface untuk Command
class Command {
public:
    virtual ~Command() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
    virtual std::string getDescription() const = 0;
};

// TextEditor class
class TextEditor {
private:
    std::string content;
    std::stack<std::shared_ptr<Command>> undoStack;
    std::stack<std::shared_ptr<Command>> redoStack;
    
    // Batas maksimum untuk undo/redo stack (opsional, untuk memory management)
    static const size_t MAX_STACK_SIZE = 100;

public:
    TextEditor() : content("") {}
    
    // Mendapatkan konten saat ini
    std::string getContent() const {
        return content;
    }
    
    // Set konten langsung (tanpa tracking undo/redo)
    void setContent(const std::string& newContent) {
        content = newContent;
    }
    
    // Execute command dan manage stacks
    void executeCommand(std::shared_ptr<Command> cmd) {
        // Clear redo stack ketika ada action baru
        while (!redoStack.empty()) {
            redoStack.pop();
        }
        
        // Execute command
        cmd->execute();
        
        // Push ke undo stack
        undoStack.push(cmd);
        
        // Limit stack size untuk memory efficiency
        if (undoStack.size() > MAX_STACK_SIZE) {
            // Remove oldest command
            std::stack<std::shared_ptr<Command>> temp;
            size_t count = 0;
            while (!undoStack.empty()) {
                if (count >= undoStack.size() - MAX_STACK_SIZE + 1) {
                    temp.push(undoStack.top());
                }
                undoStack.pop();
                count++;
            }
            undoStack = temp;
        }
    }
    
    // Undo operation
    bool undo() {
        if (undoStack.empty()) {
            std::cout << "Tidak ada aksi yang bisa di-undo." << std::endl;
            return false;
        }
        
        auto cmd = undoStack.top();
        undoStack.pop();
        
        cmd->undo();
        redoStack.push(cmd);
        
        std::cout << "Undo: " << cmd->getDescription() << std::endl;
        return true;
    }
    
    // Redo operation
    bool redo() {
        if (redoStack.empty()) {
            std::cout << "Tidak ada aksi yang bisa di-redo." << std::endl;
            return false;
        }
        
        auto cmd = redoStack.top();
        redoStack.pop();
        
        cmd->execute();
        undoStack.push(cmd);
        
        std::cout << "Redo: " << cmd->getDescription() << std::endl;
        return true;
    }
    
    // Check status
    bool canUndo() const {
        return !undoStack.empty();
    }
    
    bool canRedo() const {
        return !redoStack.empty();
    }
    
    // Clear semua history
    void clearHistory() {
        while (!undoStack.empty()) undoStack.pop();
        while (!redoStack.empty()) redoStack.pop();
    }
};

// Command untuk insert text
class InsertTextCommand : public Command {
private:
    TextEditor& editor;
    std::string text;
    size_t position;
    std::string previousContent;
    
public:
    InsertTextCommand(TextEditor& ed, const std::string& txt, size_t pos) 
        : editor(ed), text(txt), position(pos) {}
    
    void execute() override {
        previousContent = editor.getContent();
        std::string content = editor.getContent();
        
        if (position > content.length()) {
            position = content.length();
        }
        
        content.insert(position, text);
        editor.setContent(content);
    }
    
    void undo() override {
        editor.setContent(previousContent);
    }
    
    std::string getDescription() const override {
        return "Insert text: '" + text + "' at position " + std::to_string(position);
    }
};

// Command untuk delete text
class DeleteTextCommand : public Command {
private:
    TextEditor& editor;
    size_t position;
    size_t length;
    std::string deletedText;
    std::string previousContent;
    
public:
    DeleteTextCommand(TextEditor& ed, size_t pos, size_t len) 
        : editor(ed), position(pos), length(len) {}
    
    void execute() override {
        previousContent = editor.getContent();
        std::string content = editor.getContent();
        
        if (position >= content.length()) {
            return; // Nothing to delete
        }
        
        if (position + length > content.length()) {
            length = content.length() - position;
        }
        
        deletedText = content.substr(position, length);
        content.erase(position, length);
        editor.setContent(content);
    }
    
    void undo() override {
        std::string content = editor.getContent();
        content.insert(position, deletedText);
        editor.setContent(content);
    }
    
    std::string getDescription() const override {
        return "Delete text: '" + deletedText + "' from position " + std::to_string(position);
    }
};

// Command untuk replace text
class ReplaceTextCommand : public Command {
private:
    TextEditor& editor;
    size_t position;
    size_t length;
    std::string newText;
    std::string oldText;
    std::string previousContent;
    
public:
    ReplaceTextCommand(TextEditor& ed, size_t pos, size_t len, const std::string& replacement) 
        : editor(ed), position(pos), length(len), newText(replacement) {}
    
    void execute() override {
        previousContent = editor.getContent();
        std::string content = editor.getContent();
        
        if (position >= content.length()) {
            return;
        }
        
        if (position + length > content.length()) {
            length = content.length() - position;
        }
        
        oldText = content.substr(position, length);
        content.replace(position, length, newText);
        editor.setContent(content);
    }
    
    void undo() override {
        std::string content = editor.getContent();
        content.replace(position, newText.length(), oldText);
        editor.setContent(content);
    }
    
    std::string getDescription() const override {
        return "Replace text: '" + oldText + "' with '" + newText + "'";
    }
};

